#include "calc/equilibrium.h"

#include "numeric/rootFinding.h"
#include "util/error.h"

#include <format>

namespace loadicator {

namespace {

constexpr double draughtTolerance = 1e-6;
constexpr double trimTolerance = 1e-6;
constexpr double trimSearchStep = 1.0;
constexpr int maximumTrimSearchSteps = 10;

}

FloatingPosition solveEquilibrium(const MassSummary& mass, const HydrostaticTable& hydrostatics,
                                  const ShipParticulars& ship) {
    const auto draughtForTrim = [&](Length trim) {
        const auto excessBuoyancy = [&](double draught) {
            return (hydrostatics.displacementAt(Length{draught}, trim) - mass.displacement).value();
        };
        const numeric::Interval draughts = numeric::evaluate(
            excessBuoyancy, hydrostatics.minimumDraught().value(), hydrostatics.maximumDraught().value());
        if (!draughts.bracketsRoot()) {
            throw LoadicatorError(std::format(
                "a displacement of {:.1f} t is outside the hydrostatic tables, which span {:.1f} to {:.1f} t "
                "at a trim of {:.2f} m",
                mass.displacement.value(), mass.displacement.value() + draughts.lowerValue,
                mass.displacement.value() + draughts.upperValue, trim.value()));
        }
        return Length{numeric::findRoot(excessBuoyancy, draughts, draughtTolerance)};
    };
    // Horizontal distance between B and G when trimmed; zero at equilibrium. Positions are in ship
    // axes, so a trim angle θ moves each point aft by z·tanθ: B and G are on one vertical when
    // LCB − LCG = (VCB − KG)·tanθ.
    const auto lever = [&](double trimValue) {
        const Length trim{trimValue};
        const Length draught = draughtForTrim(trim);
        const Ratio tangent = trim / ship.lengthBetweenPerpendiculars;
        return (hydrostatics.lcbAt(draught, trim) - mass.centreOfGravity.x -
                (hydrostatics.vcbAt(draught) - mass.centreOfGravity.z) * tangent)
            .value();
    };

    numeric::Interval trims =
        numeric::evaluate(lever, hydrostatics.minimumTrim().value(), hydrostatics.maximumTrim().value());
    for (int step = 0; !trims.bracketsRoot(); ++step) {
        if (step == maximumTrimSearchSteps) {
            throw LoadicatorError(
                std::format("no trim equilibrium within {:.1f} .. {:.1f} m", trims.lower, trims.upper));
        }
        trims = numeric::evaluate(lever, trims.lower - trimSearchStep, trims.upper + trimSearchStep);
    }

    const Length trim{numeric::findRoot(lever, trims, trimTolerance)};
    const Length draught = draughtForTrim(trim);
    const Length draughtAft = draught + trim * (ship.referencePointX / ship.lengthBetweenPerpendiculars);
    return FloatingPosition{.draughtMidships = draught,
                            .draughtAft = draughtAft,
                            .draughtForward = draughtAft - trim,
                            .trim = trim,
                            .lcb = hydrostatics.lcbAt(draught, trim),
                            .lcf = hydrostatics.lcfAt(draught),
                            .kmt = hydrostatics.kmtAt(draught, trim),
                            .trimExtrapolated =
                                trim < hydrostatics.minimumTrim() || trim > hydrostatics.maximumTrim()};
}

}
