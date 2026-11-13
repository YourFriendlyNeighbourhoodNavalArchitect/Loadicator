#include "domain/hydrostaticTable.h"

#include "util/error.h"

#include <algorithm>
#include <set>
#include <utility>

namespace loadicator {

namespace {

[[nodiscard]] numeric::Abscissa axis(const std::vector<TrimGridPoint>& grid, Length TrimGridPoint::*member) {
    std::set<double> values;
    for (const TrimGridPoint& point : grid) {
        values.insert((point.*member).value());
    }
    return numeric::Abscissa({values.begin(), values.end()});
}

}

HydrostaticTable::SortedGrid HydrostaticTable::sorted(std::vector<TrimGridPoint> trimGrid) {
    const auto key = [](const TrimGridPoint& point) { return std::pair{point.trim, point.draught}; };
    std::ranges::sort(trimGrid, {}, key);
    numeric::Abscissa draughts = axis(trimGrid, &TrimGridPoint::draught);
    numeric::Abscissa trims = axis(trimGrid, &TrimGridPoint::trim);
    if (std::ranges::adjacent_find(trimGrid, {}, key) != trimGrid.end() ||
        trimGrid.size() != draughts.size() * trims.size()) {
        throw LoadicatorError("hydrostatic trim grid does not hold every draught at every trim exactly once");
    }
    return SortedGrid{
        .points = std::move(trimGrid), .draughts = std::move(draughts), .trims = std::move(trims)};
}

HydrostaticTable::HydrostaticTable(const std::vector<EvenKeelHydrostatics>& evenKeel,
                                   std::vector<TrimGridPoint> trimGrid)
    : HydrostaticTable(evenKeel, sorted(std::move(trimGrid))) {}

HydrostaticTable::HydrostaticTable(const std::vector<EvenKeelHydrostatics>& evenKeel, const SortedGrid& grid)
    : evenKeelDraughts(magnitudes(evenKeel, &EvenKeelHydrostatics::draught)),
      lcf(magnitudes(evenKeel, &EvenKeelHydrostatics::lcf)),
      vcb(magnitudes(evenKeel, &EvenKeelHydrostatics::vcb)), gridDraughts(grid.draughts),
      gridTrims(grid.trims),
      displacement(grid.draughts, grid.trims, magnitudes(grid.points, &TrimGridPoint::displacement)),
      lcb(grid.draughts, grid.trims, magnitudes(grid.points, &TrimGridPoint::lcb)),
      kmt(grid.draughts, grid.trims, magnitudes(grid.points, &TrimGridPoint::kmt)) {
    if (evenKeelDraughts.front() > gridDraughts.front() || evenKeelDraughts.back() < gridDraughts.back()) {
        throw LoadicatorError("the even-keel hydrostatics do not cover the draughts of the trim grid");
    }
}

Mass HydrostaticTable::displacementAt(Length draught, Length trim) const {
    return Mass{displacement.valueAt(draught.value(), trim.value())};
}

Length HydrostaticTable::lcbAt(Length draught, Length trim) const {
    return Length{lcb.valueAt(draught.value(), trim.value())};
}

Length HydrostaticTable::kmtAt(Length draught, Length trim) const {
    return Length{kmt.valueAt(draught.value(), trim.value())};
}

Length HydrostaticTable::lcfAt(Length draught) const {
    return Length{numeric::interpolate(lcf, evenKeelDraughts.locate(draught.value()))};
}

Length HydrostaticTable::vcbAt(Length draught) const {
    return Length{numeric::interpolate(vcb, evenKeelDraughts.locate(draught.value()))};
}

}
