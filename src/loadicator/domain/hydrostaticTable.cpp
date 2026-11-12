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

HydrostaticTable::HydrostaticTable(const std::vector<EvenKeelHydrostatics>& evenKeel,
                                   const std::vector<TrimGridPoint>& trimGrid)
    : evenKeelDraughts(axis(magnitudes(evenKeel, &EvenKeelHydrostatics::draught), "even-keel draughts")),
      lcf(magnitudes(evenKeel, &EvenKeelHydrostatics::lcf)),
      vcb(magnitudes(evenKeel, &EvenKeelHydrostatics::vcb)),
      gridDraughts(axis(distinctValues(trimGrid, &TrimGridPoint::draught), "grid draughts")),
      gridTrims(axis(distinctValues(trimGrid, &TrimGridPoint::trim), "grid trims")) {
    std::vector<TrimGridPoint> points = trimGrid;
    std::sort(points.begin(), points.end(), [](const TrimGridPoint& lhs, const TrimGridPoint& rhs) {
        return std::pair{lhs.trim, lhs.draught} < std::pair{rhs.trim, rhs.draught};
    });
    const std::size_t draughtCount = gridDraughts.size();
    bool complete = points.size() == draughtCount * gridTrims.size();
    for (std::size_t index = 0; complete && index < points.size(); ++index) {
        complete = points[index].trim.value() == gridTrims.nodes()[index / draughtCount] &&
                   points[index].draught.value() == gridDraughts.nodes()[index % draughtCount];
    }
    if (!complete) {
        throw LoadicatorError("hydrostatic trim grid does not hold every draught at every trim exactly once");
    }

    for (auto first = points.begin(); first != points.end();
         first += static_cast<std::ptrdiff_t>(draughtCount)) {
        const std::vector<TrimGridPoint> alongDraught(first,
                                                      first + static_cast<std::ptrdiff_t>(draughtCount));
        displacement.emplace_back(gridDraughts, magnitudes(alongDraught, &TrimGridPoint::displacement));
        lcb.emplace_back(gridDraughts, magnitudes(alongDraught, &TrimGridPoint::lcb));
        kmt.emplace_back(gridDraughts, magnitudes(alongDraught, &TrimGridPoint::kmt));
    }

    for (std::size_t node = 0; node < gridTrims.size(); ++node) {
        std::vector<double> unitAtNode(gridTrims.size(), 0.0);
        unitAtNode[node] = 1.0;
        trimBasis.emplace_back(gridTrims, std::move(unitAtNode));
    }
}

double HydrostaticTable::valueAt(const Surface& surface, Length draught, Length trim) const {
    if (draught < minimumDraught() || draught > maximumDraught()) {
        throw LoadicatorError(
            std::format("a draught of {:.3f} m is outside the hydrostatic tables ({} .. {} m)",
                        draught.value(), minimumDraught().value(), maximumDraught().value()));
    }
    if (trim < minimumTrim() || trim > maximumTrim()) {
        throw LoadicatorError(std::format("a trim of {:.3f} m is outside the hydrostatic tables ({} .. {} m)",
                                          trim.value(), minimumTrim().value(), maximumTrim().value()));
    }
    double value = 0.0;
    for (std::size_t node = 0; node < surface.size(); ++node) {
        value += trimBasis[node].valueAt(trim.value()) * surface[node].valueAt(draught.value());
    }
    return value;
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
