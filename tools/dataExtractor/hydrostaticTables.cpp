#include "hydrostaticTables.h"

#include "loadicator/util/error.h"
#include "loadicator/util/text.h"

#include <algorithm>
#include <array>
#include <map>
#include <optional>
#include <utility>

namespace loadicator::extractor {

namespace {

using Grid = std::map<std::pair<double, double>, double>;

class Reader {
public:
    explicit Reader(const SourceText& sourceText) : source(sourceText) {}

    void read(const SourceLine& line) {
        readParticulars(line);
        readTableRow(line);
    }

    [[nodiscard]] HydrostaticData result() const {
        if (!lengthBetweenPerpendiculars || !referencePointX) {
            throw source.error("the length between perpendiculars or the reference point was not found");
        }
        HydrostaticData data{.particulars = withContext(source.name(), [&] {
            return ShipParticulars(Length{*lengthBetweenPerpendiculars}, Length{*referencePointX});
        })};
        for (const auto& [point, value] : lcf) {
            data.evenKeel.push_back(EvenKeelHydrostatics{
                .draught = Length{point.first}, .lcf = Length{value}, .vcb = Length{at(vcb, point, "VCB")}});
        }
        for (const auto& [point, value] : displacement) {
            data.trimGrid.push_back(TrimGridPoint{.draught = Length{point.first},
                                                  .trim = Length{point.second},
                                                  .displacement = Mass{value},
                                                  .lcb = Length{at(lcb, point, "LCB")},
                                                  .kmt = Length{at(kmt, point, "KMT")}});
        }
        if (vcb.size() != lcf.size() || lcb.size() != displacement.size() ||
            kmt.size() != displacement.size()) {
            throw source.error("the hydrostatic properties are not tabulated at the same points");
        }
        return data;
    }

private:
    void readParticulars(const SourceLine& line) {
        if (line.text.starts_with("LENGTH BETWEEN PERPENDICULARS")) {
            lengthBetweenPerpendiculars = numberAfterColon(line);
        } else if (line.text.find("OUTPUT REFERENCE POINT (RFP)") != std::string::npos) {
            referencePointFollows = true;
        } else if (referencePointFollows && line.text.find("LONGITUDINAL") != std::string::npos) {
            referencePointX = numberAfterColon(line);
            referencePointFollows = false;
        }
    }

    void readTableRow(const SourceLine& line) {
        const auto cells = text::split(line.text, '|');
        if (cells.size() < 4 || !cells.front().empty()) {
            return;
        }
        const std::string_view label = text::trim(cells[1]);
        std::vector<double> values;
        for (std::size_t index = 2; index + 1 < cells.size(); ++index) {
            const std::string_view cell = text::trim(cells[index]);
            if (cell.empty()) {
                continue;
            }
            const auto value = text::parseNumber(cell);
            if (!value) {
                return;
            }
            values.push_back(*value);
        }

        const std::string_view name =
            label.substr(std::min(label.find_first_not_of("0123456789 "), label.size()));
        if (values.empty()) {
            const std::array<std::pair<std::string_view, Grid*>, 3> crossTables{
                {{"DISP TOTAL SW", &displacement}, {"LCB FWD OF RFP", &lcb}, {"KMT", &kmt}}};
            for (const auto& [title, table] : crossTables) {
                if (label.starts_with(title)) {
                    crossTable = table;
                }
            }
        } else if (name.starts_with("DRAUGHT RFP")) {
            draughts = std::move(values);
            crossTable = nullptr;
        } else if (name.starts_with("LCF FWD OF RFP")) {
            store(lcf, 0.0, values, referencePoint(line), line);
        } else if (name.starts_with("VCB ABOVE RFP")) {
            store(vcb, 0.0, values, 0.0, line);
        } else if (crossTable != nullptr && label.ends_with("[m]:")) {
            const auto trim = parseNumbers(label);
            if (trim.size() != 1) {
                throw source.error(line, "cannot read the trim of this row");
            }
            store(*crossTable, trim.front(), values, crossTable == &lcb ? referencePoint(line) : 0.0, line);
        }
    }

    // The offset turns a position printed forward of the reference point into one from the aft
    // perpendicular.
    void store(Grid& grid, double trim, const std::vector<double>& values, double offset,
               const SourceLine& line) {
        if (values.size() != draughts.size()) {
            throw source.error(line, std::to_string(values.size()) + " values for " +
                                         std::to_string(draughts.size()) + " draughts");
        }
        for (std::size_t column = 0; column < values.size(); ++column) {
            if (!grid.emplace(std::pair{draughts[column], trim}, values[column] + offset).second) {
                throw source.error(line, "this row is tabulated twice");
            }
        }
    }

    [[nodiscard]] double referencePoint(const SourceLine& line) const {
        if (!referencePointX) {
            throw source.error(line, "the reference point is not defined before the tables");
        }
        return *referencePointX;
    }

    [[nodiscard]] double numberAfterColon(const SourceLine& line) const {
        const auto numbers = parseNumbers(line.text.substr(std::min(line.text.find(':'), line.text.size())));
        if (numbers.empty()) {
            throw source.error(line, "expected a number after the colon");
        }
        return numbers.front();
    }

    [[nodiscard]] double at(const Grid& grid, const Grid::key_type& point, std::string_view property) const {
        const auto found = grid.find(point);
        if (found == grid.end()) {
            throw source.error("no " + std::string(property) + " at draught " + std::to_string(point.first) +
                               " m and trim " + std::to_string(point.second) + " m");
        }
        return found->second;
    }

    const SourceText& source;
    std::optional<double> lengthBetweenPerpendiculars;
    std::optional<double> referencePointX;
    bool referencePointFollows = false;
    std::vector<double> draughts;
    Grid* crossTable = nullptr;
    Grid lcf;
    Grid vcb;
    Grid displacement;
    Grid lcb;
    Grid kmt;
};

}

HydrostaticData extractHydrostatics(const SourceText& source) {
    Reader reader(source);
    for (const SourceLine& line : source.lines()) {
        reader.read(line);
    }
    return reader.result();
}

}
