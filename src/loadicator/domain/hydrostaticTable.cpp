#include "loadicator/domain/hydrostaticTable.h"

#include "loadicator/util/error.h"
#include "loadicator/util/magnitudes.h"

#include <algorithm>
#include <format>
#include <set>
#include <string>
#include <string_view>
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

[[nodiscard]] numeric::Abscissa axis(std::vector<double> values, std::string_view name) {
    return withContext("hydrostatic table, " + std::string(name),
                       [&] { return numeric::Abscissa(std::move(values)); });
}

}

HydrostaticTable::TrimGrid HydrostaticTable::orderedGrid(const std::vector<TrimGridPoint>& trimGrid) {
    TrimGrid grid{.draughts = axis(distinctValues(trimGrid, &TrimGridPoint::draught), "grid draughts"),
                  .trims = axis(distinctValues(trimGrid, &TrimGridPoint::trim), "grid trims"),
                  .points = trimGrid};
    std::sort(grid.points.begin(), grid.points.end(), [](const TrimGridPoint& lhs, const TrimGridPoint& rhs) {
        return std::pair{lhs.trim, lhs.draught} < std::pair{rhs.trim, rhs.draught};
    });
    const std::size_t draughtCount = grid.draughts.size();
    bool complete = grid.points.size() == draughtCount * grid.trims.size();
    for (std::size_t index = 0; complete && index < grid.points.size(); ++index) {
        complete = grid.points[index].trim.value() == grid.trims.nodes()[index / draughtCount] &&
                   grid.points[index].draught.value() == grid.draughts.nodes()[index % draughtCount];
    }
    if (!complete) {
        throw LoadicatorError("hydrostatic trim grid does not hold every draught at every trim exactly once");
    }
    return grid;
}

HydrostaticTable::HydrostaticTable(const std::vector<EvenKeelHydrostatics>& evenKeel,
                                   const std::vector<TrimGridPoint>& trimGrid)
    : HydrostaticTable(evenKeel, orderedGrid(trimGrid)) {}

HydrostaticTable::HydrostaticTable(const std::vector<EvenKeelHydrostatics>& evenKeel, const TrimGrid& grid)
    : evenKeelDraughts(axis(magnitudes(evenKeel, &EvenKeelHydrostatics::draught), "even-keel draughts")),
      lcf(magnitudes(evenKeel, &EvenKeelHydrostatics::lcf)),
      vcb(magnitudes(evenKeel, &EvenKeelHydrostatics::vcb)),
      gridDraughts(grid.draughts),
      gridTrims(grid.trims),
      displacement(grid.draughts, grid.trims, magnitudes(grid.points, &TrimGridPoint::displacement)),
      lcb(grid.draughts, grid.trims, magnitudes(grid.points, &TrimGridPoint::lcb)),
      kmt(grid.draughts, grid.trims, magnitudes(grid.points, &TrimGridPoint::kmt)) {
    // The LCF and the VCB are looked up at the draughts that the trim grid allows.
    if (evenKeelDraughts.front() > gridDraughts.front() || evenKeelDraughts.back() < gridDraughts.back()) {
        throw LoadicatorError(
            std::format("the even-keel hydrostatics ({} .. {} m) do not cover the draughts of the trim grid "
                        "({} .. {} m)",
                        evenKeelDraughts.front(), evenKeelDraughts.back(), gridDraughts.front(),
                        gridDraughts.back()));
    }
}

double HydrostaticTable::valueAt(const numeric::SplineSurface& surface, Length draught, Length trim) const {
    if (draught < minimumDraught() || draught > maximumDraught()) {
        throw LoadicatorError(
            std::format("a draught of {:.3f} m is outside the hydrostatic tables ({} .. {} m)",
                        draught.value(), minimumDraught().value(), maximumDraught().value()));
    }
    if (trim < minimumTrim() || trim > maximumTrim()) {
        throw LoadicatorError(std::format("a trim of {:.3f} m is outside the hydrostatic tables ({} .. {} m)",
                                          trim.value(), minimumTrim().value(), maximumTrim().value()));
    }
    return surface.valueAt(draught.value(), trim.value());
}

Mass HydrostaticTable::displacementAt(Length draught, Length trim) const {
    return Mass{valueAt(displacement, draught, trim)};
}

Length HydrostaticTable::lcbAt(Length draught, Length trim) const {
    return Length{valueAt(lcb, draught, trim)};
}

Length HydrostaticTable::kmtAt(Length draught, Length trim) const {
    return Length{valueAt(kmt, draught, trim)};
}

Length HydrostaticTable::lcfAt(Length draught) const {
    return Length{numeric::interpolate(lcf, evenKeelDraughts.locate(draught.value()))};
}

Length HydrostaticTable::vcbAt(Length draught) const {
    return Length{numeric::interpolate(vcb, evenKeelDraughts.locate(draught.value()))};
}

}
