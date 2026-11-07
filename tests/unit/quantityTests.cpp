#include "../testing.h"
#include "loadicator/units/quantity.h"

#include <numbers>
#include <type_traits>

using namespace loadicator;
using namespace loadicator::literals;

// Dimensional analysis happens at compile time; these lines would not compile if it were wrong.
static_assert(std::is_same_v<decltype(1.0_tPerM3 * 2.0_m3), Mass>);
static_assert(std::is_same_v<decltype(2.0_t * 3.0_m), Moment>);
static_assert(std::is_same_v<decltype(Moment{6.0} / 2.0_t), Length>);
static_assert(std::is_same_v<decltype(Density{1.0} * SecondMomentOfArea{1.0}), Moment>);
static_assert(!std::is_convertible_v<Mass, Length>);
static_assert(!std::is_convertible_v<double, Mass>);

TEST_CASE("units/arithmetic") {
    const Mass mass = 1.025_tPerM3 * 100.0_m3;
    CHECK_NEAR(mass.value(), 102.5, 1e-12);
    CHECK_NEAR((mass * 2.0_m).value(), 205.0, 1e-12);
    CHECK_NEAR((3.0_m - 1.0_m).value(), 2.0, 1e-12);
    CHECK_NEAR((-2.0_m).value(), -2.0, 1e-12);
    CHECK(1.0_m < 2.0_m);
    CHECK(Length{1.0} == 1.0_m);
}

TEST_CASE("units/ratioAndAngle") {
    CHECK_NEAR(ratioFromPercent(45.0).value(), 0.45, 1e-12);
    CHECK_NEAR(toPercent(Ratio{0.3}), 30.0, 1e-12);
    CHECK_NEAR(Angle::fromDegrees(180.0).radians(), std::numbers::pi, 1e-12);
    CHECK_NEAR(Angle::fromRadians(std::numbers::pi / 2.0).degrees(), 90.0, 1e-12);
}
