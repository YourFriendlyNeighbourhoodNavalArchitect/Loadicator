#include "loadingConditions.h"

#include "loadicator/core/error.h"
#include "loadicator/io/csv.h"
#include "loadicator/text/numberParsing.h"

#include <cctype>
#include <cmath>
#include <fstream>
#include <optional>

namespace loadicator::extractor {

namespace {

constexpr std::size_t compartmentRowValues = 6; // mass, fill, x, y, z, fsm
constexpr double massRoundingPerEntry = 0.05;   // masses are printed to 0.1 t

[[nodiscard]] bool isDigit(char character) {
    return std::isdigit(static_cast<unsigned char>(character)) != 0;
}

/// "R2.02S NO.2 W.B TK(S) …" ⇒ "R2.02S"; nullopt if the line does not start with an id.
[[nodiscard]] std::optional<std::string> compartmentIdAt(std::string_view text) {
    if (text.size() < 4 || text[0] != 'R' || !isDigit(text[1]) || text[2] != '.' || !isDigit(text[3])) {
        return std::nullopt;
    }
    std::size_t end = 3;
    while (end < text.size() && isDigit(text[end])) {
        ++end;
    }
    if (end < text.size() && (text[end] == 'P' || text[end] == 'S')) {
        ++end;
    }
    if (end < text.size() && text[end] != ' ') {
        return std::nullopt;
    }
    return std::string(text.substr(0, end));
}

/// The last `count` numbers of a row; descriptions such as "NO.3" may add leading noise.
[[nodiscard]] std::vector<double> trailingNumbers(std::string_view text, std::size_t count) {
    const auto numbers = text::parseNumbers(text);
    if (numbers.size() < count) {
        return {};
    }
    return {numbers.end() - static_cast<std::ptrdiff_t>(count), numbers.end()};
}

class ConditionParser {
public:
    ConditionParser(const SourceText& sourceText, const std::set<std::string, std::less<>>& tabulatedIds)
        : source(sourceText), tabulated(tabulatedIds) {}

    void parse() {
        for (const SourceLine& line : source.lines()) {
            parseLine(line, text::trim(line.text));
        }
        if (current) {
            throw LoadicatorError(source.name() + ": COND" + std::to_string(current->number) +
                                  " has no floating position");
        }
    }

    [[nodiscard]] std::vector<LoadingConditionData> takeConditions() { return std::move(conditions); }

private:
    void parseLine(const SourceLine& line, std::string_view text) {
        constexpr std::string_view heading = "LOADING CONDITION - COND";
        if (text.starts_with(heading)) {
            if (current) {
                throw LoadicatorError(source.where(line, "previous condition has no floating position"));
            }
            const auto colon = text.find(':');
            current = LoadingConditionData{
                .number = static_cast<int>(text::parseNumbers(text.substr(heading.size(), 2)).at(0)),
                .title =
                    colon == std::string_view::npos ? "" : std::string(text::trim(text.substr(colon + 1)))};
            density.reset();
            solid = false;
            return;
        }
        if (!current) {
            return;
        }
        if (parseConditionLine(*current, line, text)) {
            checkTotals(*current, line);
            conditions.push_back(std::move(*current));
            current.reset();
        }
    }

    /// Handles one line inside a condition block; returns true at the block's last line.
    bool parseConditionLine(LoadingConditionData& condition, const SourceLine& line, std::string_view text) {

        if (text.starts_with("CONTENTS=")) {
            const std::size_t rho = text.find("RHO=");
            if (rho == std::string_view::npos) {
                throw LoadicatorError(source.where(line, "CONTENTS line without RHO"));
            }
            density = text::parseNumbers(text.substr(rho + 4)).at(0);
            solid = text.find("Solid") != std::string_view::npos;
        } else if (const auto id = compartmentIdAt(text)) {
            addCompartment(condition, line, *id, text.substr(id->size()));
        } else if (text.starts_with("(CREW&ST.") || text.starts_with("(OIL&WAT.")) {
            const std::string name = text.starts_with("(CREW") ? "CREW&ST" : "OIL&WAT";
            addWeight(condition, line, name, trailingNumbers(text.substr(9), compartmentRowValues));
        } else if (text.starts_with("Lightweight")) {
            const auto values = expectTokens(line, text, 4);
            condition.weights.push_back(
                WeightEntry{"Lightweight", values[0], values[1], values[2], values[3], 0.0});
        } else if (text.starts_with("Total weight")) {
            const auto values = expectTokens(line, text, 4);
            condition.result.displacement = values[0];
            condition.result.lcg = values[1];
            condition.result.tcg = values[2];
            condition.result.kg = values[3];
        } else if (text.starts_with("Draught moulded")) {
            const auto values = expectTokens(line, text, 2);
            condition.result.draught = values[0];
            condition.result.kmt = values[1];
        } else if (text.starts_with("Trim ") && !text.starts_with("Trimming")) {
            condition.result.trim = expectTokens(line, text, 2)[0];
        } else if (text.starts_with("Heel")) {
            condition.result.heel = expectTokens(line, text, 1)[0];
        } else if (text.starts_with("TA ")) {
            const auto values = expectTokens(line, text, 2);
            condition.result.draughtAft = values[0];
            condition.result.gmSolid = values[1];
        } else if (text.starts_with("TF ")) {
            const auto values = expectTokens(line, text, 2);
            condition.result.draughtForward = values[0];
            condition.result.gmCorrection = values[1];
        } else if (text.starts_with("Trimming moment")) {
            condition.result.gm = expectTokens(line, text, 2)[1];
            return true;
        }
        return false;
    }

    void addCompartment(LoadingConditionData& condition, const SourceLine& line, const std::string& id,
                        std::string_view rest) {
        const auto values = trailingNumbers(rest, compartmentRowValues);
        if (values.empty()) {
            throw LoadicatorError(source.where(line, "cannot read the values of " + id));
        }
        if (!tabulated.contains(id)) {
            addWeight(condition, line, id, values);
            return;
        }
        if (!density) {
            throw LoadicatorError(source.where(line, id + " is not preceded by a CONTENTS line"));
        }
        condition.tanks.push_back(TankEntry{id, values[0], *density, solid});
    }

    void addWeight(LoadingConditionData& condition, const SourceLine& line, const std::string& id,
                   const std::vector<double>& values) {
        if (values.size() != compartmentRowValues) {
            throw LoadicatorError(source.where(line, "cannot read the values of " + id));
        }
        condition.weights.push_back(WeightEntry{id, values[0], values[2], values[3], values[4], values[5]});
    }

    [[nodiscard]] std::vector<double> expectTokens(const SourceLine& line, std::string_view text,
                                                   std::size_t count) const {
        auto values = numericTokens(text);
        if (values.size() != count) {
            throw LoadicatorError(source.where(line, "expected " + std::to_string(count) + " numbers"));
        }
        return values;
    }

    void checkTotals(const LoadingConditionData& condition, const SourceLine& line) const {
        double totalMass = 0.0;
        for (const auto& tank : condition.tanks) {
            totalMass += tank.mass;
        }
        for (const auto& weight : condition.weights) {
            totalMass += weight.mass;
        }
        const std::size_t entries = condition.tanks.size() + condition.weights.size();
        const double tolerance = massRoundingPerEntry * static_cast<double>(entries + 1);
        if (std::abs(totalMass - condition.result.displacement) > tolerance) {
            throw LoadicatorError(
                source.where(line, "COND" + std::to_string(condition.number) + " entries sum to " +
                                       std::to_string(totalMass) + " t, book total is " +
                                       std::to_string(condition.result.displacement) + " t"));
        }
    }

    const SourceText& source;
    const std::set<std::string, std::less<>>& tabulated;
    std::vector<LoadingConditionData> conditions;
    std::optional<LoadingConditionData> current;
    std::optional<double> density;
    bool solid = false;
};

[[nodiscard]] std::string conditionName(int number) {
    return std::string(number < 10 ? "COND0" : "COND") + std::to_string(number);
}

[[nodiscard]] std::string fileStem(int number) {
    return std::string(number < 10 ? "cond0" : "cond") + std::to_string(number);
}

} // namespace

std::vector<LoadingConditionData>
extractLoadingConditions(const SourceText& source,
                         const std::set<std::string, std::less<>>& tabulatedCompartments) {
    ConditionParser parser(source, tabulatedCompartments);
    parser.parse();
    auto conditions = parser.takeConditions();
    if (conditions.empty()) {
        throw LoadicatorError(source.name() + ": no loading conditions found");
    }
    return conditions;
}

void writeLoadingConditions(const std::vector<LoadingConditionData>& conditions,
                            const std::filesystem::path& outputDirectory) {
    const std::filesystem::path directory = outputDirectory / "conditions";
    std::filesystem::create_directories(directory);

    for (const LoadingConditionData& condition : conditions) {
        std::ofstream file(directory / (fileStem(condition.number) + ".csv"), std::ios::binary);
        io::CsvWriter writer(file);
        writer.metadata("name", conditionName(condition.number) + " " + condition.title);
        writer.metadata("source", "data/source/trimAndStabilityBook.pdf");
        writer.comment("kind: tank (capacity table; give mass or fillPercent, plus density and phase)");
        writer.comment(
            "      weight (fixed item; give mass, lcg [m from AP], tcg [m, + port], vcg [m], fsm [t*m])");
        writer.header({"kind", "id", "mass", "fillPercent", "density", "phase", "lcg", "tcg", "vcg", "fsm"});
        for (const TankEntry& tank : condition.tanks) {
            const std::vector<std::string> fields{"tank",
                                                  tank.id,
                                                  io::formatNumber(tank.mass, 1),
                                                  "",
                                                  io::formatNumber(tank.density, 3),
                                                  tank.solid ? "solid" : "liquid",
                                                  "",
                                                  "",
                                                  "",
                                                  ""};
            writer.row(fields);
        }
        for (const WeightEntry& weight : condition.weights) {
            const std::vector<std::string> fields{"weight",
                                                  weight.id,
                                                  io::formatNumber(weight.mass, 1),
                                                  "",
                                                  "",
                                                  "",
                                                  io::formatNumber(weight.lcg, 2),
                                                  io::formatNumber(weight.tcg, 2),
                                                  io::formatNumber(weight.vcg, 2),
                                                  io::formatNumber(weight.fsm, 2)};
            writer.row(fields);
        }
    }

    std::ofstream file(directory / "expected.csv", std::ios::binary);
    io::CsvWriter writer(file);
    writer.metadata("source", "data/source/trimAndStabilityBook.pdf (Total weight, FLOATING POSITION)");
    writer.comment("Results printed in the book; used as regression reference. Heel and tcg + to port.");
    writer.header({"condition", "displacement", "lcg", "tcg", "kg", "draught", "trim", "heel", "draughtAft",
                   "draughtForward", "kmt", "gmSolid", "gmCorrection", "gm"});
    for (const LoadingConditionData& condition : conditions) {
        const BookResult& result = condition.result;
        const std::vector<std::string> fields{fileStem(condition.number),
                                              io::formatNumber(result.displacement, 1),
                                              io::formatNumber(result.lcg, 2),
                                              io::formatNumber(result.tcg, 2),
                                              io::formatNumber(result.kg, 2),
                                              io::formatNumber(result.draught, 3),
                                              io::formatNumber(result.trim, 3),
                                              io::formatNumber(result.heel, 1),
                                              io::formatNumber(result.draughtAft, 3),
                                              io::formatNumber(result.draughtForward, 3),
                                              io::formatNumber(result.kmt, 2),
                                              io::formatNumber(result.gmSolid, 2),
                                              io::formatNumber(result.gmCorrection, 2),
                                              io::formatNumber(result.gm, 2)};
        writer.row(fields);
    }
}

} // namespace loadicator::extractor
