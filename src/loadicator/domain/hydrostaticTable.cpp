#include "loadicator/domain/hydrostaticTable.h"

#include "loadicator/core/error.h"
#include "loadicator/numeric/spline.h"

#include <algorithm>
#include <set>
#include <utility>

namespace loadicator {

namespace {

[[nodiscard]] std::vector<double> distinctValues(const std::vector<TrimGridPoint>& grid,
                                                 Length TrimGridPoint::*member) {
    std::set<double> values;
    for (const auto& point : grid) {
        values.insert((point.*member).value());
    }
    return {values.begin(), values.end()};
}

template <typename Selector>
[[nodiscard]] std::vector<double> evenKeelColumn(const std::vector<EvenKeelHydrostatics>& rows,
                                                 Selector select) {
    std::vector<double> values;
    values.reserve(rows.size());
    for (const auto& row : rows) {
        values.push_back(select(row).value());
    }
    return values;
}

[[nodiscard]] numeric::Abscissa namedAbscissa(std::vector<double> values, const char* name) {
    try {
        return numeric::Abscissa(std::move(values));
    } catch (const LoadicatorError& error) {
        throw LoadicatorError(std::string("hydrostatic table, ") + name + ": " + error.what());
    }
}

} // namespace

HydrostaticTable::HydrostaticTable(const std::vector<EvenKeelHydrostatics>& evenKeel,
                                   const std::vector<TrimGridPoint>& trimGrid)
    : evenKeelColumns{evenKeelColumn(evenKeel, [](const auto& row) { return row.draught; }),
                      evenKeelColumn(evenKeel, [](const auto& row) { return row.displacement; }),
                      evenKeelColumn(evenKeel, [](const auto& row) { return row.lcf; }),
                      evenKeelColumn(evenKeel, [](const auto& row) { return row.lcb; }),
                      evenKeelColumn(evenKeel, [](const auto& row) { return row.vcb; }),
                      evenKeelColumn(evenKeel, [](const auto& row) { return row.kmt; }),
                      evenKeelColumn(evenKeel, [](const auto& row) { return row.mctOneCm; })},
      evenKeelDraughts(namedAbscissa(evenKeelColumns.draught, "even-keel draughts")),
      evenKeelDisplacements(namedAbscissa(evenKeelColumns.displacement, "even-keel displacements")),
      gridDraughts(namedAbscissa(distinctValues(trimGrid, &TrimGridPoint::draught), "grid draughts")),
      gridTrims(namedAbscissa(distinctValues(trimGrid, &TrimGridPoint::trim), "grid trims")) {
    const std::size_t trimCount = gridTrims.size();
    const std::size_t draughtCount = gridDraughts.size();
    if (trimGrid.size() != draughtCount * trimCount) {
        throw LoadicatorError("hydrostatic trim grid is incomplete: " + std::to_string(trimGrid.size()) +
                              " points for " + std::to_string(draughtCount * trimCount) + " cells");
    }

    // Column-major [trim][draught] values, placed by looking up each point's grid indices.
    const auto indexOf = [](const numeric::Abscissa& axis, double value) {
        const auto nodes = axis.nodes();
        return static_cast<std::size_t>(std::lower_bound(nodes.begin(), nodes.end(), value) - nodes.begin());
    };
    std::vector<std::vector<double>> displacement(trimCount, std::vector<double>(draughtCount));
    std::vector<std::vector<double>> lcb = displacement;
    std::vector<std::vector<double>> kmt = displacement;
    std::set<std::pair<std::size_t, std::size_t>> filled;
    for (const auto& point : trimGrid) {
        const std::size_t trimIndex = indexOf(gridTrims, point.trim.value());
        const std::size_t draughtIndex = indexOf(gridDraughts, point.draught.value());
        if (!filled.emplace(trimIndex, draughtIndex).second) {
            throw LoadicatorError("hydrostatic trim grid has a duplicate point");
        }
        displacement[trimIndex][draughtIndex] = point.values.displacement.value();
        lcb[trimIndex][draughtIndex] = point.values.lcb.value();
        kmt[trimIndex][draughtIndex] = point.values.kmt.value();
    }
    for (std::size_t trimIndex = 0; trimIndex < trimCount; ++trimIndex) {
        trimColumns.push_back(TrimColumn{
            .displacement = numeric::NaturalCubicSpline(gridDraughts, std::move(displacement[trimIndex])),
            .lcb = numeric::NaturalCubicSpline(gridDraughts, std::move(lcb[trimIndex])),
            .kmt = numeric::NaturalCubicSpline(gridDraughts, std::move(kmt[trimIndex]))});
    }
}

EvenKeelHydrostatics HydrostaticTable::evenKeelRow(numeric::Bracket bracket) const {
    const auto at = [&](const std::vector<double>& column) {
        return numeric::interpolate(column, bracket);
    };
    return EvenKeelHydrostatics{.draught = Length{at(evenKeelColumns.draught)},
                                .displacement = Mass{at(evenKeelColumns.displacement)},
                                .lcf = Length{at(evenKeelColumns.lcf)},
                                .lcb = Length{at(evenKeelColumns.lcb)},
                                .vcb = Length{at(evenKeelColumns.vcb)},
                                .kmt = Length{at(evenKeelColumns.kmt)},
                                .mctOneCm = Moment{at(evenKeelColumns.mctOneCm)}};
}

EvenKeelHydrostatics HydrostaticTable::evenKeelAt(Mass displacement) const {
    return evenKeelRow(evenKeelDisplacements.locate(displacement.value()));
}

EvenKeelHydrostatics HydrostaticTable::evenKeelAtDraught(Length draught) const {
    return evenKeelRow(evenKeelDraughts.locate(draught.value()));
}

TrimmedHydrostatics HydrostaticTable::trimmedAt(Length draught, Length trim) const {
    (void)gridDraughts.locate(draught.value()); // rejects draughts outside the tables

    const auto valueAt = [&](numeric::NaturalCubicSpline TrimColumn::*quantity) {
        std::vector<double> alongTrim;
        alongTrim.reserve(trimColumns.size());
        for (const TrimColumn& column : trimColumns) {
            alongTrim.push_back((column.*quantity).valueAt(draught.value()));
        }
        return numeric::NaturalCubicSpline(gridTrims, std::move(alongTrim)).valueAt(trim.value());
    };
    return TrimmedHydrostatics{.displacement = Mass{valueAt(&TrimColumn::displacement)},
                               .lcb = Length{valueAt(&TrimColumn::lcb)},
                               .kmt = Length{valueAt(&TrimColumn::kmt)}};
}

} // namespace loadicator
