#include "../testing.h"
#include "loadicator/numeric/interpolation.h"
#include "loadicator/numeric/rootFinding.h"
#include "loadicator/numeric/spline.h"

#include <cmath>
#include <vector>

using namespace loadicator::numeric;

TEST_CASE("numeric/bracketsWithBinarySearch") {
    const Abscissa nodes({0.0, 1.0, 3.0, 6.0});
    const Bracket inside = nodes.locate(2.0);
    CHECK(inside.lower == 1);
    CHECK_NEAR(inside.weight, 0.5, 1e-12);
    CHECK(nodes.locate(0.0).lower == 0);
    CHECK(nodes.locate(6.0).lower == 2);
    CHECK_NEAR(nodes.locate(6.0).weight, 1.0, 1e-12);
}

TEST_CASE("numeric/outOfRangePolicies") {
    const Abscissa nodes({0.0, 1.0, 2.0});
    CHECK_THROWS(nodes.locate(2.5));
    CHECK_NEAR(interpolate(std::vector{0.0, 10.0, 20.0}, nodes.locate(-1.0, OutOfRange::clamp)), 0.0, 1e-12);
    CHECK_NEAR(interpolate(std::vector{0.0, 10.0, 20.0}, nodes.locate(3.0, OutOfRange::extrapolate)), 30.0,
               1e-12);
}

TEST_CASE("numeric/rejectsUnsortedNodes") {
    CHECK_THROWS(Abscissa({0.0}));
    CHECK_THROWS(Abscissa({0.0, 1.0, 1.0}));
    CHECK_THROWS(Abscissa({0.0, 2.0, 1.0}));
}

TEST_CASE("numeric/usesBracketingRowsNotNearestRows") {
    // Audit C7: hold 1, V = 2000 m3. The original "two nearest rows" search picked rows 0 and 2
    // (0 and 3374.1 m3) and returned VCG 3.073 m; the bracketing rows give 4.141 m.
    const std::vector volumes{0.0, 1687.0, 3374.1, 5061.1};
    const std::vector vcg{0.000, 3.903, 5.184, 6.335};
    CHECK_NEAR(interpolateLinear(volumes, vcg, 2000.0), 4.141, 5e-4);
}

TEST_CASE("numeric/splineReproducesLinesAndNodes") {
    const Abscissa nodes({-2.0, 0.0, 2.0, 4.0, 6.0});
    const NaturalCubicSpline line(nodes, {-3.0, 1.0, 5.0, 9.0, 13.0});
    CHECK_NEAR(line.valueAt(1.3), 3.6, 1e-12);
    CHECK_NEAR(line.valueAt(8.0), 17.0, 1e-12); // linear extrapolation
    const NaturalCubicSpline curve(nodes, {4.0, 0.0, 4.0, 16.0, 36.0});
    CHECK_NEAR(curve.valueAt(2.0), 4.0, 1e-12);
    CHECK(std::abs(curve.valueAt(3.0) - 9.0) < 0.1); // close to x² between nodes
}

TEST_CASE("numeric/findRoot") {
    const double root = findRoot([](double x) { return x * x - 2.0; }, 0.0, 2.0, 1e-12);
    CHECK_NEAR(root, std::sqrt(2.0), 1e-9);
    CHECK_NEAR(findRoot([](double x) { return x - 1.0; }, 1.0, 3.0, 1e-12), 1.0, 0.0);
    CHECK_THROWS(findRoot([](double x) { return x * x + 1.0; }, -1.0, 1.0, 1e-12));
}
