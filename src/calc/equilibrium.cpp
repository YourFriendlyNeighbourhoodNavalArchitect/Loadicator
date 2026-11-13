#include "calc/equilibrium.h"

#include "util/error.h"

#include <cstddef>
#include <format>
#include <optional>
#include <span>
#include <utility>
#include <vector>

namespace loadicator {

namespace {

constexpr double tolerance = 1e-6;

// Narrows [lower, upper], at whose ends the predicate differs, by bisection until it is shorter than the
// tolerance. Each end keeps the value of the predicate it had.
template <typename Predicate>
[[nodiscard]] std::pair<double, double> bisect(const Predicate& predicate, double lower, double upper) {
    const bool atLower = predicate(lower);
    while (upper - lower > tolerance) {
        const double middle = (lower + upper) / 2.0;
        (predicate(middle) == atLower ? lower : upper) = middle;
    }
    return {lower, upper};
}

struct TrimRange {
    double lower = 0.0;
    double upper = 0.0;
};

// The ranges of the tabulated trims in which the predicate holds. It is taken to change at most once between
// neighbouring tabulated trims, which is the resolution at which the tables describe the ship.
template <typename Predicate>
[[nodiscard]] std::vector<TrimRange> trimsWhere(const Predicate& holds, std::span<const double> trims) {
    std::vector<TrimRange> ranges;
    std::optional<double> start;
    if (holds(trims.front())) {
        start = trims.front();
    }
    for (std::size_t index = 1; index < trims.size(); ++index) {
        if (holds(trims[index]) == start.has_value()) {
            continue;
        }
        const auto [before, after] = bisect(holds, trims[index - 1], trims[index]);
        if (start) {
            ranges.push_back(TrimRange{*start, before});
            start.reset();
        } else {
            start = after;
        }
    }
    if (start) {
        ranges.push_back(TrimRange{*start, trims.back()});
    }
    return ranges;
}

}

FloatingPosition solveEquilibrium(Mass displacement, const Position& centreOfGravity,
                                  const HydrostaticTable& hydrostatics, const ShipParticulars& ship) {
    const double minimumDraught = hydrostatics.minimumDraught().value();
    const double maximumDraught = hydrostatics.maximumDraught().value();
    const auto excessBuoyancy = [&](double draught, double trim) {
        return (hydrostatics.displacementAt(Length{draught}, Length{trim}) - displacement).value();
    };

    // The draught for a trim lies within the tables only where the displacement lies between those of the
    // least and of the greatest tabulated draught at that trim, so the trim is sought only there.
    const auto floats = [&](double trim) {
        return excessBuoyancy(minimumDraught, trim) <= 0.0 && excessBuoyancy(maximumDraught, trim) >= 0.0;
    };
    const std::span<const double> tabulatedTrims = hydrostatics.tabulatedTrims();
    const std::vector<TrimRange> floatingTrims = trimsWhere(floats, tabulatedTrims);
    if (floatingTrims.empty()) {
        throw LoadicatorError(std::format("a displacement of {:.1f} t lies outside the hydrostatic tables at "
                                          "every tabulated trim from {} to {} m",
                                          displacement.value(), tabulatedTrims.front(),
                                          tabulatedTrims.back()));
    }

    const auto draughtForTrim = [&](double trim) {
        const auto [lower, upper] =
            bisect([&](double draught) { return excessBuoyancy(draught, trim) > 0.0; }, minimumDraught,
                   maximumDraught);
        return Length{(lower + upper) / 2.0};
    };
    // Horizontal distance between B and G when trimmed; zero at equilibrium. Positions are in ship axes, so
    // a trim angle θ moves each point aft by z·tanθ: B and G are on one vertical when
    // LCB − LCG = (VCB − KG)·tanθ.
    const auto lever = [&](double trimValue) {
        const Length trim{trimValue};
        const Length draught = draughtForTrim(trimValue);
        const Ratio tangent = trim / ship.lengthBetweenPerpendiculars;
        return (hydrostatics.lcbAt(draught, trim) - centreOfGravity.x -
                (hydrostatics.vcbAt(draught) - centreOfGravity.z) * tangent)
            .value();
    };
    const auto buoyancyForward = [&](double trim) { return lever(trim) >= 0.0; };

    for (const TrimRange& range : floatingTrims) {
        if (buoyancyForward(range.lower) == buoyancyForward(range.upper)) {
            continue;
        }
        const auto [lower, upper] = bisect(buoyancyForward, range.lower, range.upper);
        const Length trim{(lower + upper) / 2.0};
        const Length draught = draughtForTrim(trim.value());
        const Length draughtAft = draught + trim * (ship.referencePointX / ship.lengthBetweenPerpendiculars);
        return FloatingPosition{.draughtMidships = draught,
                                .draughtAft = draughtAft,
                                .draughtForward = draughtAft - trim,
                                .trim = trim,
                                .lcb = hydrostatics.lcbAt(draught, trim),
                                .lcf = hydrostatics.lcfAt(draught),
                                .kmt = hydrostatics.kmtAt(draught, trim)};
    }
    throw LoadicatorError("no trim equilibrium at the trims where the hydrostatic tables float the ship");
}

}
