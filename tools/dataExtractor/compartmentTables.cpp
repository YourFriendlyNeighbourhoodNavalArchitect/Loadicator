#include "compartmentTables.h"

#include "loadicator/core/error.h"
#include "loadicator/io/csv.h"
#include "loadicator/text/numberParsing.h"

#include <algorithm>
#include <cmath>
#include <deque>
#include <fstream>
#include <optional>

namespace loadicator::extractor {

namespace {

constexpr std::size_t completeRowFields = 13; // sounding, 7 volumes, fill, lcg, tcg, vcg, imom
constexpr std::size_t truncatedRowFields = 8; // sounding, 7 volumes
constexpr std::size_t tailFields = 5;         // fill, lcg, tcg, vcg, imom
constexpr double fillTolerancePercent = 0.15; // FILL is printed to 0.1 %
constexpr double centimetresPerMetre = 100.0;

[[nodiscard]] bool isDashLine(std::string_view text) {
    return text.size() >= 3 && text.find_first_not_of('-') == std::string_view::npos;
}

/// Splits "35.798.6" into "35.7" and "98.6" (one decimal on the glued IMOM).
[[nodiscard]] std::vector<std::string_view> unglue(std::string_view token) {
    const std::size_t firstDot = token.find('.');
    if (firstDot == std::string_view::npos || token.find('.', firstDot + 1) == std::string_view::npos) {
        return {token};
    }
    const std::size_t splitAt = firstDot + 2;
    return {token.substr(0, splitAt), token.substr(splitAt)};
}

[[nodiscard]] std::optional<std::vector<double>> parseDataLine(std::string_view text) {
    std::vector<double> values;
    for (const std::string_view token : tokenize(text)) {
        for (const std::string_view piece : unglue(token)) {
            const auto value = text::parseNumber(piece);
            if (!value) {
                return std::nullopt;
            }
            values.push_back(*value);
        }
    }
    return values;
}

struct SoundingRow {
    std::vector<double> values; // complete rows have 13 values
    std::size_t lineNumber = 0;
};

[[nodiscard]] CapacityRow toCapacityRow(const std::vector<double>& values) {
    return CapacityRow{.sounding = values[0] / centimetresPerMetre,
                       .volume = values[1],
                       .lcg = values[9],
                       .tcg = values[10],
                       .vcg = values[11],
                       .inertia = values[12]};
}

/// Collects one compartment's data block, repairing truncated rows.
class SoundingBlockParser {
public:
    SoundingBlockParser(const SourceText& sourceText, CompartmentTableData& target)
        : source(sourceText), table(target) {}

    void addLine(const SourceLine& line, std::string_view text) {
        auto values = parseDataLine(text);
        if (!values) {
            throw LoadicatorError(source.where(line, "unrecognised data line in " + table.id));
        }
        if (!carry.empty()) {
            values->insert(values->begin(), carry.begin(), carry.end());
            carry.clear();
        }
        const bool isTailLine = !pending.empty() && values->size() % tailFields == 0;
        if (values->size() < truncatedRowFields && !isTailLine) {
            carry = std::move(*values); // a row split by a page break: "190" / "1048.72 … 58265.2"
            return;
        }
        if (values->size() == completeRowFields - 1 && rows.empty() && values->front() == 0.0) {
            values->insert(values->begin(), 0.0); // empty-tank row printed without its zero sounding
        }
        if (values->size() == completeRowFields) {
            rows.push_back({*values, line.number});
        } else if (values->size() == truncatedRowFields) {
            pending.push_back(rows.size());
            rows.push_back({*values, line.number});
        } else if (!values->empty() && values->size() % tailFields == 0 &&
                   values->size() / tailFields <= pending.size()) {
            for (std::size_t offset = 0; offset < values->size(); offset += tailFields) {
                auto& row = rows[pending.front()].values;
                row.insert(row.end(), values->begin() + static_cast<std::ptrdiff_t>(offset),
                           values->begin() + static_cast<std::ptrdiff_t>(offset + tailFields));
                pending.pop_front();
            }
        } else {
            throw LoadicatorError(source.where(line, "cannot interpret " + std::to_string(values->size()) +
                                                         " values in " + table.id));
        }
    }

    void finish(const SourceLine& line) {
        if (!carry.empty()) {
            throw LoadicatorError(source.where(line, "incomplete row at the end of " + table.id));
        }
        if (!pending.empty()) {
            throw LoadicatorError(source.where(line, std::to_string(pending.size()) + " truncated rows of " +
                                                         table.id + " were never completed"));
        }
        if (rows.size() < 2) {
            throw LoadicatorError(source.where(line, table.id + " has fewer than two rows"));
        }
        // FILL is relative to the full capacity, which some tables stop short of (last row 99.9 %).
        const auto fullest = std::max_element(rows.begin(), rows.end(), [](const auto& lhs, const auto& rhs) {
            return lhs.values[1] < rhs.values[1];
        });
        const double capacity = fullest->values[1] * 100.0 / fullest->values[8];
        table.capacity = capacity;
        for (const auto& row : rows) {
            const double expectedFill = 100.0 * row.values[1] / capacity;
            if (std::abs(row.values[8] - expectedFill) > fillTolerancePercent) {
                throw LoadicatorError(source.name() + ":" + std::to_string(row.lineNumber) + ": " + table.id +
                                      " FILL " + std::to_string(row.values[8]) +
                                      " % disagrees with volume (" + std::to_string(expectedFill) + " %)");
            }
            table.rows.push_back(toCapacityRow(row.values));
        }
    }

private:
    const SourceText& source;
    CompartmentTableData& table;
    std::vector<SoundingRow> rows;
    std::deque<std::size_t> pending; // indices of truncated rows awaiting their tails
    std::vector<double> carry;       // leading fragment of a row continued on the next line
};

/// Parses the data lines of one compartment, closed by the dashed line `terminator`.
void parseSoundingBlock(const SourceText& source, CompartmentTableData& table,
                        const std::vector<const SourceLine*>& dataLines, const SourceLine& terminator) {
    SoundingBlockParser parser(source, table);
    for (const SourceLine* line : dataLines) {
        parser.addLine(*line, text::trim(line->text));
    }
    parser.finish(terminator);
}

[[nodiscard]] std::string valueAfterColon(std::string_view text) {
    const std::size_t colon = text.find(':');
    return std::string(text::trim(text.substr(colon + 1)));
}

} // namespace

std::vector<CompartmentTableData> extractSoundingTables(const SourceText& source) {
    enum class Section { seekingCompartment, header, data };

    std::vector<CompartmentTableData> tables;
    std::vector<const SourceLine*> dataLines;
    Section section = Section::seekingCompartment;
    bool sawUnitsRow = false;

    for (const SourceLine& line : source.lines()) {
        const std::string_view text = text::trim(line.text);

        if (text.starts_with("Compartment ident:")) {
            if (section == Section::data) {
                throw LoadicatorError(source.where(line, tables.back().id + " is not terminated"));
            }
            tables.push_back(CompartmentTableData{.id = valueAfterColon(text),
                                                  .provenance = "data/source/soundingTables.txt (trim 0 m)"});
            dataLines.clear();
            section = Section::header;
            sawUnitsRow = false;
            continue;
        }

        switch (section) {
        case Section::seekingCompartment:
            break;
        case Section::header:
            if (text.starts_with("Compartment descr:")) {
                tables.back().description = valueAfterColon(text);
            } else if (text.starts_with("Contents")) {
                tables.back().contents = valueAfterColon(text);
            } else if (text.starts_with("CM ")) {
                sawUnitsRow = true;
            } else if (sawUnitsRow && isDashLine(text)) {
                section = Section::data;
            }
            break;
        case Section::data:
            if (isDashLine(text)) {
                parseSoundingBlock(source, tables.back(), dataLines, line);
                section = Section::seekingCompartment;
            } else if (!text.empty()) {
                dataLines.push_back(&line);
            }
            break;
        }
    }
    if (section != Section::seekingCompartment) {
        throw LoadicatorError(source.name() + ": last compartment is not terminated");
    }
    return tables;
}

CompartmentTableData extractHoldTable(const SourceText& source, int holdNumber) {
    constexpr std::size_t holdFields = 6; // sounding, volume, lcg, tcg, vcg, fsm (inertia)

    CompartmentTableData table{.id = "R1." + std::to_string(holdNumber),
                               .description = "NO." + std::to_string(holdNumber) + " CARGO HOLD",
                               .contents = "Cargo hold",
                               .provenance =
                                   "data/source/cargoHolds/hold" + std::to_string(holdNumber) + ".txt"};
    for (const SourceLine& line : source.lines()) {
        const auto values = parseDataLine(text::trim(line.text));
        if (!values || values->empty()) {
            continue; // title and column-legend lines
        }
        if (values->size() != holdFields) {
            throw LoadicatorError(source.where(line, "expected 6 values per hold row"));
        }
        table.rows.push_back(CapacityRow{.sounding = (*values)[0],
                                         .volume = (*values)[1],
                                         .lcg = (*values)[2],
                                         .tcg = (*values)[3],
                                         .vcg = (*values)[4],
                                         .inertia = (*values)[5]});
    }
    if (table.rows.size() < 2) {
        throw LoadicatorError(source.name() + ": hold table has fewer than two rows");
    }
    table.capacity = table.rows.back().volume;
    return table;
}

void writeCompartmentTable(const CompartmentTableData& table, const std::filesystem::path& directory) {
    std::ofstream file(directory / (table.id + ".csv"), std::ios::binary);
    if (!file) {
        throw LoadicatorError("cannot write compartment table for " + table.id);
    }
    io::CsvWriter writer(file);
    writer.metadata("id", table.id);
    writer.metadata("description", table.description);
    writer.metadata("contents", table.contents);
    writer.metadata("capacity", io::formatNumber(table.capacity, 2));
    writer.metadata("source", table.provenance);
    writer.comment("sounding [m], volume [m3], lcg [m from AP], tcg [m, + port], vcg [m above BL], "
                   "inertia [m4, free-surface second moment of area]; capacity [m3] = volume at 100 % fill");
    writer.header({"sounding", "volume", "lcg", "tcg", "vcg", "inertia"});

    double previousVolume = -1.0;
    for (const CapacityRow& row : table.rows) {
        if (row.volume <= previousVolume) {
            continue;
        }
        previousVolume = row.volume;
        const std::vector<std::string> fields{
            io::formatNumber(row.sounding, 3), io::formatNumber(row.volume, 2),
            io::formatNumber(row.lcg, 3),      io::formatNumber(row.tcg, 3),
            io::formatNumber(row.vcg, 3),      io::formatNumber(row.inertia, 1)};
        writer.row(fields);
    }
}

} // namespace loadicator::extractor
