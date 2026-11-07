#include "../testing.h"
#include "loadicator/calc/equilibrium.h"

#include <vector>

using namespace loadicator;
using namespace loadicator::literals;

namespace {

// Rectangular barge, L = 100 m, B = 20 m, reference point amidships. For a box the trimmed
// hydrostatics are analytic: Δ = ρLBT (independent of trim), LCB = L/2 − tL/(12T),
// KMT = T/2 + B²/(12T), MCT1cm = Δ·L/(1200T).
constexpr double length = 100.0;
constexpr double breadth = 20.0;
constexpr double density = 1.025;

ShipParticulars barge() {
    return ShipParticulars{Length{length}, Length{breadth}, 10.0_m, 8.0_m, Length{length / 2.0}};
}

HydrostaticTable bargeHydrostatics() {
    std::vector<EvenKeelHydrostatics> evenKeel;
    std::vector<TrimGridPoint> grid;
    for (double draught = 1.0; draught <= 10.0; draught += 0.5) {
        const double displacement = density * length * breadth * draught;
        const double kmt = draught / 2.0 + breadth * breadth / (12.0 * draught);
        evenKeel.push_back({Length{draught}, Mass{displacement}, Length{length / 2.0}, Length{length / 2.0},
                            Length{draught / 2.0}, Length{kmt},
                            Moment{displacement * length / (1200.0 * draught)}});
        for (double trim = -2.0; trim <= 6.0; trim += 2.0) {
            grid.push_back(
                {Length{draught}, Length{trim},
                 TrimmedHydrostatics{Mass{displacement},
                                     Length{length / 2.0 - trim * length / (12.0 * draught)}, Length{kmt}}});
        }
    }
    return HydrostaticTable(evenKeel, grid);
}

MassSummary bargeLoad(double kg) {
    // T = 5 m, G 0.5 m aft of amidships ⇒ trim t = 12·T·0.5/L = 0.3 m by the stern.
    return MassSummary{Mass{density * length * breadth * 5.0}, Position{49.5_m, 0.0_m, Length{kg}},
                       Moment{0.0}};
}

} // namespace

TEST_CASE("equilibrium/firstOrderBoxBarge") {
    const auto position =
        solveEquilibrium(bargeLoad(2.5), bargeHydrostatics(), barge(), EquilibriumMethod::firstOrder);
    CHECK_NEAR(position.trim.value(), 0.3, 1e-9);
    CHECK_NEAR(position.draughtAft.value(), 5.15, 1e-9);
    CHECK_NEAR(position.draughtForward.value(), 4.85, 1e-9);
    CHECK_NEAR(position.draughtMidships.value(), 5.0, 1e-9);
}

TEST_CASE("equilibrium/freeTrimBoxBarge") {
    // With G at the height of B the trimmed attitude adds no lever: same answer as first order.
    const auto level =
        solveEquilibrium(bargeLoad(2.5), bargeHydrostatics(), barge(), EquilibriumMethod::freeTrim);
    CHECK_NEAR(level.trim.value(), 0.3, 1e-6);
    CHECK_NEAR(level.draughtMidships.value(), 5.0, 1e-6);
    CHECK_NEAR(level.draughtAft.value(), 5.15, 1e-6);
    CHECK(!level.trimExtrapolated);

    // With G 7.5 m above B, B must lie 7.5·tanθ aft of G: 0.5 − t·L/60 = −7.5·t/L ⇒ t = 0.5 / (5/3 − 0.075).
    const auto high =
        solveEquilibrium(bargeLoad(10.0), bargeHydrostatics(), barge(), EquilibriumMethod::freeTrim);
    CHECK_NEAR(high.trim.value(), 0.5 / (5.0 / 3.0 - 0.075), 1e-6);
}

TEST_CASE("equilibrium/rejectsDisplacementOutsideTables") {
    MassSummary tooHeavy = bargeLoad(2.5);
    tooHeavy.displacement = Mass{density * length * breadth * 12.0};
    CHECK_THROWS(solveEquilibrium(tooHeavy, bargeHydrostatics(), barge(), EquilibriumMethod::firstOrder));
    CHECK_THROWS(solveEquilibrium(tooHeavy, bargeHydrostatics(), barge(), EquilibriumMethod::freeTrim));
}
