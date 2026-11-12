#include "loadicator/calc/equilibrium.h"

#include "loadicator/numeric/rootFinding.h"
#include "loadicator/util/error.h"

#include <algorithm>
#include <cstddef>
#include <format>
#include <optional>
#include <span>
#include <vector>

namespace loadicator {

namespace {

constexpr double draughtTolerance = 1e-6;
constexpr double trimTolerance = 1e-6;

struct TrimRange {
    double lower = 0.0;
    double upper = 0.0;
};

// The ranges of trim in which the function is not negative. The function is taken to change sign at most once
// between neighbouring tabulated trims, which is the resolution at which the tables describe the ship. Each
// end of a range is a trim at which the function is not negative.
template <typename Function>
[[nodiscard]] std::vector<TrimRange> trimsWhereNotNegative(const Function& function,
                                                           std::span<const double> tabulatedTrims) {
    std::vector<TrimRange> ranges;
    std::optional<double> start;
    double previous = tabulatedTrims.front();
    double previousValue = function(previous);
    if (previousValue >= 0.0) {
        start = previous;
    }
    for (std::size_t index = 1; index < tabulatedTrims.size(); ++index) {
        const double trim = tabulatedTrims[index];
        const double value = function(trim);
        if ((previousValue >= 0.0) != (value >= 0.0)) {
            const numeric::Interval change = numeric::narrowBracket(
                function, numeric::Interval{previous, trim, previousValue, value}, trimTolerance);
            const double boundary = change.lowerValue >= 0.0 ? change.lower : change.upper;
            if (value >= 0.0) {
                start = boundary;
            } else {
                ranges.push_back(TrimRange{*start, boundary});
                start.reset();
            }
        }
        previous = trim;
        previousValue = value;
    }
    if (start) {
        ranges.push_back(TrimRange{*start, tabulatedTrims.back()});
    }
    return ranges;
}

[[nodiscard]] std::vector<TrimRange> intersection(const std::vector<TrimRange>& first,
                                                  const std::vector<TrimRange>& second) {
    std::vector<TrimRange> common;
    auto lhs = first.begin();
    auto rhs = second.begin();
    while (lhs != first.end() && rhs != second.end()) {
        const double lower = std::max(lhs->lower, rhs->lower);
        const double upper = std::min(lhs->upper, rhs->upper);
        if (lower <= upper) {
            common.push_back(TrimRange{lower, upper});
        }
        if (lhs->upper < rhs->upper) {
            ++lhs;
        } else {
            ++rhs;
        }
    }
    return common;
}

}

FloatingPosition solveEquilibrium(const MassSummary& mass, const HydrostaticTable& hydrostatics,
                                  const ShipParticulars& ship) {
    const double minimumDraught = hydrostatics.minimumDraught().value();
    const double maximumDraught = hydrostatics.maximumDraught().value();
    const auto excessBuoyancy = [&](double draught, double trim) {
        return (hydrostatics.displacementAt(Length{draught}, Length{trim}) - mass.displacement).value();
    };

    // The draught for a trim lies within the tables only where the displacement lies between those of the
    // least and of the greatest tabulated draught at that trim, so the trim is sought only there.
    const std::vector<TrimRange> floatingTrims = intersection(
        trimsWhereNotNegative([&](double trim) { return -excessBuoyancy(minimumDraught, trim); },
                              hydrostatics.tabulatedTrims()),
        trimsWhereNotNegative([&](double trim) { return excessBuoyancy(maximumDraught, trim); },
                              hydrostatics.tabulatedTrims()));
    if (floatingTrims.empty()) {
        throw LoadicatorError(std::format("a displacement of {:.1f} t lies outside the hydrostatic tables at "
                                          "every trim within {} .. {} m",
                                          mass.displacement.value(), hydrostatics.minimumTrim().value(),
                                          hydrostatics.maximumTrim().value()));
    }

    const auto draughtForTrim = [&](double trim) {
        const auto atTrim = [&](double draught) { return excessBuoyancy(draught, trim); };
        return Length{numeric::findRoot(atTrim, numeric::evaluate(atTrim, minimumDraught, maximumDraught),
                                        draughtTolerance)};
    };
    // Horizontal distance between B and G when trimmed; zero at equilibrium. Positions are in ship
    // axes, so a trim angle θ moves each point aft by z·tanθ: B and G are on one vertical when
    // LCB − LCG = (VCB − KG)·tanθ.
    const auto lever = [&](double trimValue) {
        const Length trim{trimValue};
        const Length draught = draughtForTrim(trimValue);
        const Ratio tangent = trim / ship.lengthBetweenPerpendiculars();
        return (hydrostatics.lcbAt(draught, trim) - mass.centreOfGravity.x -
                (hydrostatics.vcbAt(draught) - mass.centreOfGravity.z) * tangent)
            .value();
    };

    for (const TrimRange& range : floatingTrims) {
        const numeric::Interval trims = numeric::evaluate(lever, range.lower, range.upper);
        if (!trims.bracketsRoot()) {
            continue;
        }
        const Length trim{numeric::findRoot(lever, trims, trimTolerance)};
        const Length draught = draughtForTrim(trim.value());
        const Length draughtAft =
            draught + trim * (ship.referencePointX() / ship.lengthBetweenPerpendiculars());
        return FloatingPosition{.draughtMidships = draught,
                                .draughtAft = draughtAft,
                                .draughtForward = draughtAft - trim,
                                .trim = trim,
                                .lcb = hydrostatics.lcbAt(draught, trim),
                                .lcf = hydrostatics.lcfAt(draught),
                                .kmt = hydrostatics.kmtAt(draught, trim)};
    }
    throw LoadicatorError(std::format("no trim equilibrium within the hydrostatic tables ({} .. {} m)",
                                      hydrostatics.minimumTrim().value(),
                                      hydrostatics.maximumTrim().value()));
}

}
