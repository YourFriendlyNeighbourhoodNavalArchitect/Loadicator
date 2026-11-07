#include "../testing.h"
#include "loadicator/calc/massSummary.h"
#include "loadicator/calc/stability.h"

#include <cmath>
#include <vector>

using namespace loadicator;
using namespace loadicator::literals;

TEST_CASE("stability/summaryKeepsFreeSurfaceOutOfTcg") {
    const std::vector<WeightItem> items{
        {"a", 100.0_t, Position{10.0_m, 0.0_m, 2.0_m}, Moment{500.0}},
        {"b", 300.0_t, Position{30.0_m, 0.0_m, 6.0_m}, Moment{0.0}},
    };
    const MassSummary summary = summarizeWeights(items);
    CHECK_NEAR(summary.displacement.value(), 400.0, 1e-12);
    CHECK_NEAR(summary.centreOfGravity.x.value(), 25.0, 1e-12);
    CHECK_NEAR(summary.centreOfGravity.z.value(), 5.0, 1e-12);
    // Audit C6: the original added FSM to the transverse moment, inventing a heel.
    CHECK_NEAR(summary.centreOfGravity.y.value(), 0.0, 0.0);
    CHECK_NEAR(summary.freeSurfaceMoment.value(), 500.0, 1e-12);
}

TEST_CASE("stability/rejectsEmptyShip") {
    // Audit C12: zero displacement only printed a warning.
    CHECK_THROWS(summarizeWeights(std::vector<WeightItem>{}));
}

TEST_CASE("stability/freeSurfaceReducesGm") {
    const MassSummary summary{1000.0_t, Position{50.0_m, 0.05_m, 8.0_m}, Moment{2000.0}};
    const InitialStability stability = evaluateInitialStability(summary, 10.0_m);
    CHECK_NEAR(stability.gmSolid.value(), 2.0, 1e-12);
    CHECK_NEAR(stability.freeSurfaceCorrection.value(), 2.0 * 1.0, 1e-12);
    CHECK(!stability.heel.has_value()); // GM fluid = 0 ⇒ unstable, heel undefined (audit C13)

    const InitialStability stable = evaluateInitialStability(summary, 12.0_m);
    CHECK_NEAR(stable.gmFluid.value(), 2.0, 1e-12);
    CHECK(stable.heel.has_value());
    if (stable.heel) {
        CHECK_NEAR(stable.heel->radians(), std::atan(0.05 / 2.0), 1e-12); // positive to port, like TCG
    }
}
