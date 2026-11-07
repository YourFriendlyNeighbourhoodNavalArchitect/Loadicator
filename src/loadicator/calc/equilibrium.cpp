#include "loadicator/calc/equilibrium.h"

#include "loadicator/core/error.h"
#include "loadicator/numeric/rootFinding.h"

#include <cmath>
#include <utility>

namespace loadicator {

namespace {

constexpr double centimetresPerMetre = 100.0;
constexpr double draughtTolerance = 1e-6; // m
constexpr double trimTolerance = 1e-6;    // m
constexpr double trimSearchStep = 1.0;    // m, widening step when the root lies beyond the grid
constexpr int maximumTrimSearchSteps = 10;

/// Draughts at the perpendiculars for a draught measured at referencePointX.
struct PerpendicularDraughts {
    Length aft;
    Length forward;
};

[[nodiscard]] PerpendicularDraughts draughtsAtPerpendiculars(Length draughtAtPoint, Length pointX,
                                                             Length trim, const ShipParticulars& ship) {
    const Length aft = draughtAtPoint + trim * (pointX / ship.lengthBetweenPerpendiculars).value();
    return {aft, aft - trim};
}

[[nodiscard]] FloatingPosition solveFirstOrder(const MassSummary& mass, const HydrostaticTable& hydrostatics,
                                               const ShipParticulars& ship) {
    const EvenKeelHydrostatics evenKeel = hydrostatics.evenKeelAt(mass.displacement);
    const Moment trimmingMoment = mass.displacement * (evenKeel.lcb - mass.centreOfGravity.x);
    const Length trim{(trimmingMoment / evenKeel.mctOneCm).value() / centimetresPerMetre};

    // The ship trims about the centre of flotation, where the even-keel draught is kept.
    const auto draughts = draughtsAtPerpendiculars(evenKeel.draught, evenKeel.lcf, trim, ship);
    const Length draughtMidships =
        draughts.aft - trim * (ship.referencePointX / ship.lengthBetweenPerpendiculars).value();

    return FloatingPosition{.method = EquilibriumMethod::firstOrder,
                            .draughtMidships = draughtMidships,
                            .draughtAft = draughts.aft,
                            .draughtForward = draughts.forward,
                            .trim = trim,
                            .lcb = evenKeel.lcb,
                            .lcf = evenKeel.lcf,
                            .kmt = evenKeel.kmt};
}

[[nodiscard]] FloatingPosition solveFreeTrim(const MassSummary& mass, const HydrostaticTable& hydrostatics,
                                             const ShipParticulars& ship) {
    const double minimumDraught = hydrostatics.minimumDraught().value();
    const double maximumDraught = hydrostatics.maximumDraught().value();

    // Draught at the reference point that supports the displacement at a given trim.
    const auto draughtForTrim = [&](double trim) {
        const auto excessBuoyancy = [&](double draught) {
            return (hydrostatics.trimmedAt(Length{draught}, Length{trim}).displacement - mass.displacement)
                .value();
        };
        return numeric::findRoot(excessBuoyancy, minimumDraught, maximumDraught, draughtTolerance);
    };
    // Horizontal distance between B and G when trimmed; zero at equilibrium. Positions are in
    // ship axes, so a trim angle θ moves each point aft by z·tanθ (higher points further):
    // B and G are on one vertical when LCB − LCG = (VCB − KG)·tanθ.
    const auto lever = [&](double trim) {
        const Length draught{draughtForTrim(trim)};
        const double tangent = trim / ship.lengthBetweenPerpendiculars.value();
        const Length vcb = hydrostatics.evenKeelAtDraught(draught).vcb;
        const Length lcb = hydrostatics.trimmedAt(draught, Length{trim}).lcb;
        return (lcb - mass.centreOfGravity.x - (vcb - mass.centreOfGravity.z) * tangent).value();
    };

    double lowerTrim = hydrostatics.minimumTrim().value();
    double upperTrim = hydrostatics.maximumTrim().value();
    for (int step = 0; std::signbit(lever(lowerTrim)) == std::signbit(lever(upperTrim)); ++step) {
        if (step == maximumTrimSearchSteps) {
            throw LoadicatorError("no trim equilibrium within " + std::to_string(lowerTrim) + " .. " +
                                  std::to_string(upperTrim) + " m");
        }
        lowerTrim -= trimSearchStep;
        upperTrim += trimSearchStep;
    }

    const Length trim{numeric::findRoot(lever, lowerTrim, upperTrim, trimTolerance)};
    const Length draught{draughtForTrim(trim.value())};
    const TrimmedHydrostatics atEquilibrium = hydrostatics.trimmedAt(draught, trim);
    const auto draughts = draughtsAtPerpendiculars(draught, ship.referencePointX, trim, ship);

    return FloatingPosition{.method = EquilibriumMethod::freeTrim,
                            .draughtMidships = draught,
                            .draughtAft = draughts.aft,
                            .draughtForward = draughts.forward,
                            .trim = trim,
                            .lcb = atEquilibrium.lcb,
                            .lcf = hydrostatics.evenKeelAtDraught(draught).lcf,
                            .kmt = atEquilibrium.kmt,
                            .trimExtrapolated =
                                trim < hydrostatics.minimumTrim() || trim > hydrostatics.maximumTrim()};
}

} // namespace

std::string_view toString(EquilibriumMethod method) {
    switch (method) {
    case EquilibriumMethod::firstOrder:
        return "first order (even-keel hydrostatics, rotation about LCF)";
    case EquilibriumMethod::freeTrim:
        return "free trim (draught x trim hydrostatic grid)";
    }
    return "unknown";
}

FloatingPosition solveEquilibrium(const MassSummary& mass, const HydrostaticTable& hydrostatics,
                                  const ShipParticulars& ship, EquilibriumMethod method) {
    switch (method) {
    case EquilibriumMethod::firstOrder:
        return solveFirstOrder(mass, hydrostatics, ship);
    case EquilibriumMethod::freeTrim:
        return solveFreeTrim(mass, hydrostatics, ship);
    }
    throw LoadicatorError("unknown equilibrium method");
}

} // namespace loadicator
