#include "compartmentTables.h"

#include "loadicator/util/text.h"

#include <algorithm>
#include <cmath>
#include <optional>
#include <utility>

namespace loadicator::extractor {

namespace {

constexpr std::size_t volumeColumn = 1;

// A row of the sounding tables: sounding [cm], volume [m3] at seven trims (zero trim first), fill [%],
// lcg, tcg, vcg [m] and free-surface inertia [m4].
namespace sounding {
constexpr std::size_t fill = 8;
constexpr std::size_t centroid = 9;
constexpr std::size_t startFields = 8;
constexpr std::size_t endFields = 5;
constexpr std::size_t fields = startFields + endFields;
}

// A row of the cargo-hold tables: sounding [m], volume [m3], lcg, tcg, vcg [m] and inertia [m4].
namespace hold {
constexpr std::size_t centroid = 2;
constexpr std::size_t fields = 6;
}

constexpr double fillTolerance = 0.15;

using Values = std::vector<double>;

struct DataLine {
    const SourceLine* source = nullptr;
    Values values;
};

// A token with two decimal points is an inertia glued to the fill that follows it ("35.798.6"). The
// inertia is printed with one decimal, so it ends one digit after its point.
[[nodiscard]] std::optional<Values> numbersOf(std::string_view text) {
    Values values;
    const auto append = [&](std::string_view token) {
        const auto value = text::parseNumber(token);
        if (value) {
            values.push_back(*value);
        }
        return value.has_value();
    };
    for (std::string_view token : tokenize(text)) {
        const std::size_t point = token.find('.');
        if (point != std::string_view::npos && token.find('.', point + 1) != std::string_view::npos) {
            if (!append(token.substr(0, point + 2))) {
                return std::nullopt;
            }
            token.remove_prefix(point + 2);
        }
        if (!append(token)) {
            return std::nullopt;
        }
    }
    return values;
}

// Repair: a page break leaves the start of a row alone on its line. Such a fragment is shorter than
// any other line, and is put back in front of the line that continues it.
[[nodiscard]] std::vector<DataLine> joinBrokenRows(const SourceText& source, std::vector<DataLine> lines) {
    std::vector<DataLine> joined;
    DataLine fragment;
    for (DataLine& line : lines) {
        line.values.insert(line.values.begin(), fragment.values.begin(), fragment.values.end());
        fragment = {};
        if (line.values.size() < sounding::endFields) {
            fragment = std::move(line);
        } else {
            joined.push_back(std::move(line));
        }
    }
    if (fragment.source != nullptr) {
        throw source.error(*fragment.source, "the table ends in the middle of a row");
    }
    return joined;
}

// Repair: the row of the empty tank is printed without its sounding of zero in some tables.
void restoreZeroSounding(std::vector<DataLine>& lines) {
    if (!lines.empty() && lines.front().values.size() == sounding::fields - 1) {
        lines.front().values.insert(lines.front().values.begin(), 0.0);
    }
}

// Repair: a run of rows cut after their volumes is followed by one line that holds the ends of all of
// them, in order.
[[nodiscard]] std::vector<DataLine> completeCutRows(const SourceText& source, std::vector<DataLine> lines) {
    std::vector<DataLine> rows;
    std::size_t cutRows = 0;
    for (DataLine& line : lines) {
        const std::size_t count = line.values.size();
        if (count == sounding::startFields) {
            ++cutRows;
            rows.push_back(std::move(line));
        } else if (cutRows == 0 && count == sounding::fields) {
            rows.push_back(std::move(line));
        } else if (cutRows > 0 && count == cutRows * sounding::endFields) {
            auto end = line.values.begin();
            for (auto row = rows.end() - static_cast<std::ptrdiff_t>(cutRows); row != rows.end(); ++row) {
                row->values.insert(row->values.end(), end, end + sounding::endFields);
                end += sounding::endFields;
            }
            cutRows = 0;
        } else {
            throw source.error(*line.source, "cannot interpret a line of " + std::to_string(count) +
                                                 " values after " + std::to_string(cutRows) + " cut rows");
        }
    }
    if (cutRows > 0) {
        throw source.error(*rows.back().source, "the ends of the last rows are missing");
    }
    return rows;
}

void keepIncreasingVolumes(std::vector<CompartmentTable::Row>& rows) {
    std::vector<CompartmentTable::Row> increasing;
    for (const CompartmentTable::Row& row : rows) {
        if (increasing.empty() || row.volume > increasing.back().volume) {
            increasing.push_back(row);
        }
    }
    rows = std::move(increasing);
}

[[nodiscard]] CompartmentTable::Row tableRow(const Values& values, std::size_t centroid) {
    return CompartmentTable::Row{.volume = Volume{values[volumeColumn]},
                                 .centroid = Position{Length{values[centroid]}, Length{values[centroid + 1]},
                                                      Length{values[centroid + 2]}},
                                 .freeSurfaceInertia = SecondMomentOfArea{values[centroid + 3]}};
}

[[nodiscard]] CompartmentData soundingTable(const SourceText& source, std::string id,
                                            std::vector<DataLine> lines) {
    lines = joinBrokenRows(source, std::move(lines));
    restoreZeroSounding(lines);
    const std::vector<DataLine> rows = completeCutRows(source, std::move(lines));
    if (rows.size() < 2) {
        throw source.error(id + " has fewer than two rows");
    }

    const Values& fullest =
        std::max_element(rows.begin(), rows.end(), [](const DataLine& lhs, const DataLine& rhs) {
            return lhs.values[volumeColumn] < rhs.values[volumeColumn];
        })->values;
    // The fill is relative to the capacity, which a table that stops short of 100 % does not print.
    const double capacity = fullest[volumeColumn] * 100.0 / fullest[sounding::fill];

    CompartmentData table{.id = std::move(id)};
    for (const DataLine& row : rows) {
        const double fill = 100.0 * row.values[volumeColumn] / capacity;
        if (std::abs(row.values[sounding::fill] - fill) > fillTolerance) {
            throw source.error(*row.source, table.id + ": the printed fill of " +
                                                std::to_string(row.values[sounding::fill]) +
                                                " % disagrees with the volume (" + std::to_string(fill) +
                                                " %)");
        }
        table.rows.push_back(tableRow(row.values, sounding::centroid));
    }
    keepIncreasingVolumes(table.rows);
    // Such a table is closed with a row at its capacity, so that its last volume is the full tank.
    if (fullest[sounding::fill] < 100.0) {
        table.rows.push_back(table.rows.back());
        table.rows.back().volume = Volume{capacity};
    }
    return table;
}

// Repair: the empty hold is printed with its centre at (0, 0, 0). It takes the centre of the first
// cargo: lengthwise and across as in the next row, and at the height that the next two rows reach when
// continued in a straight line to zero volume.
void placeEmptyHoldCentroid(std::vector<CompartmentTable::Row>& rows) {
    CompartmentTable::Row& empty = rows[0];
    const CompartmentTable::Row& first = rows[1];
    const CompartmentTable::Row& second = rows[2];
    if (empty.volume != Volume{0.0}) {
        return;
    }
    const Length risePerRow = second.centroid.z - first.centroid.z;
    empty.centroid =
        Position{first.centroid.x, first.centroid.y,
                 first.centroid.z - risePerRow * (first.volume / (second.volume - first.volume))};
}

[[nodiscard]] std::optional<std::string> valueAfter(std::string_view line, std::string_view label) {
    if (!line.starts_with(label)) {
        return std::nullopt;
    }
    return std::string(text::trim(line.substr(label.size())));
}

[[nodiscard]] bool isRule(std::string_view line) {
    return line.size() >= 3 && line.find_first_not_of('-') == std::string_view::npos;
}

}

std::vector<CompartmentData> extractSoundingTables(const SourceText& source) {
    struct Block {
        std::string id;
        std::vector<DataLine> lines;
    };
    std::vector<CompartmentData> tables;
    std::optional<Block> block;
    bool inData = false;

    for (const SourceLine& line : source.lines()) {
        if (auto id = valueAfter(line.text, "Compartment ident:")) {
            if (block) {
                throw source.error(line, "the table of " + block->id + " has no end");
            }
            block = Block{.id = std::move(*id)};
        } else if (!block) {
            continue;
        } else if (isRule(line.text)) {
            if (inData) {
                tables.push_back(soundingTable(source, std::move(block->id), std::move(block->lines)));
                block.reset();
            }
            inData = !inData;
        } else if (inData && !line.text.empty()) {
            auto values = numbersOf(line.text);
            if (!values) {
                throw source.error(line,
                                   "a data line of " + block->id + " holds something that is not a number");
            }
            block->lines.push_back(DataLine{&line, std::move(*values)});
        }
    }
    if (block) {
        throw source.error("the table of " + block->id + " has no end");
    }
    if (tables.empty()) {
        throw source.error("no capacity tables found");
    }
    return tables;
}

CompartmentData extractHoldTable(const SourceText& source) {
    const auto title = std::find_if(source.lines().begin(), source.lines().end(),
                                    [](const SourceLine& line) { return !line.text.empty(); });
    const Values holdNumber = title == source.lines().end() ? Values{} : parseNumbers(title->text);
    if (holdNumber.size() != 1) {
        throw source.error("the first line does not name the hold");
    }
    const std::string number = std::to_string(static_cast<int>(holdNumber.front()));

    CompartmentData table{.id = "R1." + number};
    for (auto line = title + 1; line != source.lines().end(); ++line) {
        const auto values = numbersOf(line->text);
        if (!values || values->empty()) {
            continue;
        }
        if (values->size() != hold::fields) {
            throw source.error(*line, "expected " + std::to_string(hold::fields) + " values in a row");
        }
        table.rows.push_back(tableRow(*values, hold::centroid));
    }
    keepIncreasingVolumes(table.rows);
    if (table.rows.size() < 3) {
        throw source.error("the table has fewer than three rows");
    }
    placeEmptyHoldCentroid(table.rows);
    return table;
}

}
