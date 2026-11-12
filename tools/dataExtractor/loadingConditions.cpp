#include "loadingConditions.h"

#include "loadicator/book/bookResults.h"
#include "loadicator/util/text.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <optional>
#include <utility>

namespace loadicator::extractor {

namespace {

namespace results = book::column;

constexpr std::string_view heading = "LOADING CONDITION - COND";

struct ResultLabel {
    std::string_view label;
    std::string_view column;
};

constexpr std::array floatingPosition{
    ResultLabel{"moulded", results::draught},   ResultLabel{"KM", results::kmt},
    ResultLabel{"Trim", results::trim},         ResultLabel{"PS=+", results::heel},
    ResultLabel{"TA", results::draughtAft},     ResultLabel{"GM0", results::gmSolid},
    ResultLabel{"TF", results::draughtForward}, ResultLabel{"GM", results::gm}};

constexpr std::array totalWeight{results::displacement, results::lcg, results::tcg, results::kg};

constexpr std::size_t entryValues = 6;

[[nodiscard]] bool isCompartmentId(std::string_view token) {
    const auto isDigit = [](char character) {
        return std::isdigit(static_cast<unsigned char>(character)) != 0;
    };
    if (token.size() < 4 || token[0] != 'R' || !isDigit(token[1]) || token[2] != '.') {
        return false;
    }
    if (token.back() == 'P' || token.back() == 'S') {
        token.remove_suffix(1);
    }
    return token.size() > 3 && std::all_of(token.begin() + 3, token.end(), isDigit);
}

class Reader {
public:
    Reader(const SourceText& sourceText, const std::set<std::string, std::less<>>& tabulatedCompartments)
        : source(sourceText), tabulated(tabulatedCompartments) {}

    void read(const SourceLine& line) {
        const std::vector<std::string_view> tokens = tokenize(line.text);
        if (line.text.starts_with(heading)) {
            begin(line);
        } else if (!current || tokens.empty()) {
            return;
        } else if (line.text.starts_with("CONTENTS=")) {
            readContents(line);
        } else if (isCompartmentId(tokens.front())) {
            addEntry(line, std::string(tokens.front()));
        } else if (tokens.front().starts_with('(')) {
            addWeight(line, std::string(tokens.front().substr(1, tokens.front().size() - 2)));
        } else if (tokens.front() == "Lightweight") {
            const auto values = numbers(line, 4);
            current->condition.fixedWeights.push_back(
                WeightItem{.name = "Lightweight",
                           .mass = Mass{values[0]},
                           .centre = Position{Length{values[1]}, Length{values[2]}, Length{values[3]}},
                           .freeSurfaceMoment = Moment{0.0}});
        } else if (line.text.starts_with("Total weight")) {
            const auto values = numbers(line, totalWeight.size());
            for (std::size_t index = 0; index < totalWeight.size(); ++index) {
                current->results[totalWeight[index]] = values[index];
            }
        } else if (!current->results.empty()) {
            readFloatingPosition(line, tokens);
        }
    }

    [[nodiscard]] std::vector<BookCondition> result() {
        if (current) {
            throw source.error("the last condition has no floating position");
        }
        if (conditions.empty()) {
            throw source.error("no loading conditions found");
        }
        return std::move(conditions);
    }

private:
    void begin(const SourceLine& line) {
        if (current) {
            throw source.error(line, "the previous condition has no floating position");
        }
        const std::string_view rest = std::string_view(line.text).substr(heading.size());
        const std::size_t colon = rest.find(':');
        const auto number = text::parseNumber(rest.substr(0, colon));
        if (colon == std::string_view::npos || !number) {
            throw source.error(line, "cannot read the number and the title of the condition");
        }
        current = BookCondition{.number = static_cast<int>(*number)};
        current->condition.name = "COND" + std::string(text::trim(rest.substr(0, colon))) + " " +
                                  std::string(text::trim(rest.substr(colon + 1)));
        density.reset();
    }

    void readContents(const SourceLine& line) {
        const std::size_t rho = line.text.find("RHO=");
        const auto values =
            rho == std::string::npos ? std::vector<double>{} : parseNumbers(line.text.substr(rho));
        if (values.empty()) {
            throw source.error(line, "the contents have no density");
        }
        density = Density{values.front()};
        phase = line.text.find("Solid") == std::string::npos ? CargoPhase::liquid : CargoPhase::solid;
    }

    void addEntry(const SourceLine& line, const std::string& id) {
        if (!tabulated.contains(id)) {
            addWeight(line, id);
            return;
        }
        if (!density) {
            throw source.error(line, id + " is not preceded by its contents");
        }
        const double mass = entry(line).front();
        current->condition.tankLoads.emplace_back(CompartmentId{id}, Mass{mass}, *density, phase);
    }

    void addWeight(const SourceLine& line, const std::string& name) {
        const auto values = entry(line);
        current->condition.fixedWeights.push_back(
            WeightItem{.name = name,
                       .mass = Mass{values[0]},
                       .centre = Position{Length{values[2]}, Length{values[3]}, Length{values[4]}},
                       .freeSurfaceMoment = Moment{values[5]}});
    }

    // The values of an entry are its last numbers: the description before them may hold digits, and a
    // negative value may be glued to the value before it.
    [[nodiscard]] std::vector<double> entry(const SourceLine& line) const {
        const auto values = parseNumbers(std::string_view(line.text).substr(line.text.find(' ') + 1));
        if (values.size() < entryValues) {
            throw source.error(line, "expected " + std::to_string(entryValues) + " values in an entry");
        }
        return {values.end() - static_cast<std::ptrdiff_t>(entryValues), values.end()};
    }

    void readFloatingPosition(const SourceLine& line, const std::vector<std::string_view>& tokens) {
        for (std::size_t index = 0; index + 1 < tokens.size(); ++index) {
            const auto found =
                std::find_if(floatingPosition.begin(), floatingPosition.end(),
                             [&](const ResultLabel& entry) { return entry.label == tokens[index]; });
            const auto value = text::parseNumber(tokens[index + 1]);
            if (found == floatingPosition.end() || !value) {
                continue;
            }
            current->results[found->column] = *value;
            if (found->column == floatingPosition.back().column) {
                finish(line);
                return;
            }
        }
    }

    void finish(const SourceLine& line) {
        if (current->results.size() != floatingPosition.size() + totalWeight.size()) {
            throw source.error(line, current->condition.name + ": some results of the book were not found");
        }
        conditions.push_back(std::move(*current));
        current.reset();
    }

    [[nodiscard]] std::vector<double> numbers(const SourceLine& line, std::size_t count) const {
        auto values = parseNumbers(line.text);
        if (values.size() != count) {
            throw source.error(line, "expected " + std::to_string(count) + " numbers");
        }
        return values;
    }

    const SourceText& source;
    const std::set<std::string, std::less<>>& tabulated;
    std::vector<BookCondition> conditions;
    std::optional<BookCondition> current;
    std::optional<Density> density;
    CargoPhase phase = CargoPhase::liquid;
};

}

std::vector<BookCondition>
extractLoadingConditions(const SourceText& source,
                         const std::set<std::string, std::less<>>& tabulatedCompartments) {
    Reader reader(source, tabulatedCompartments);
    for (const SourceLine& line : source.lines()) {
        reader.read(line);
    }
    return reader.result();
}

}
