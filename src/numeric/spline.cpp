#include "numeric/spline.h"

#include "util/error.h"

#include <utility>

namespace loadicator::numeric {

NaturalCubicSpline::NaturalCubicSpline(Abscissa nodes, std::vector<double> values)
    : abscissa(std::move(nodes)), ordinates(std::move(values)), curvatures(abscissa.size(), 0.0) {
    const auto x = abscissa.nodes();
    const std::size_t count = x.size();
    if (ordinates.size() != count) {
        throw LoadicatorError("spline needs one ordinate per node");
    }
    if (count < 3) {
        return;
    }

    std::vector<double> diagonal(count, 0.0);
    std::vector<double> upper(count, 0.0);
    std::vector<double> rightHandSide(count, 0.0);
    for (std::size_t index = 1; index + 1 < count; ++index) {
        const double before = x[index] - x[index - 1];
        const double after = x[index + 1] - x[index];
        diagonal[index] = 2.0 * (before + after);
        upper[index] = after;
        rightHandSide[index] = 6.0 * ((ordinates[index + 1] - ordinates[index]) / after -
                                      (ordinates[index] - ordinates[index - 1]) / before);
        if (index > 1) {
            const double factor = before / diagonal[index - 1];
            diagonal[index] -= factor * upper[index - 1];
            rightHandSide[index] -= factor * rightHandSide[index - 1];
        }
    }
    for (std::size_t index = count - 2; index >= 1; --index) {
        curvatures[index] = (rightHandSide[index] - upper[index] * curvatures[index + 1]) / diagonal[index];
    }
}

double NaturalCubicSpline::valueAt(double x) const {
    const auto nodes = abscissa.nodes();
    const std::size_t last = nodes.size() - 1;
    if (x < nodes.front()) {
        const double width = nodes[1] - nodes[0];
        const double slope = (ordinates[1] - ordinates[0]) / width - width * curvatures[1] / 6.0;
        return ordinates[0] + slope * (x - nodes[0]);
    }
    if (x > nodes.back()) {
        const double width = nodes[last] - nodes[last - 1];
        const double slope =
            (ordinates[last] - ordinates[last - 1]) / width + width * curvatures[last - 1] / 6.0;
        return ordinates[last] + slope * (x - nodes[last]);
    }

    const std::size_t lower = abscissa.locate(x).lower;
    const double width = nodes[lower + 1] - nodes[lower];
    const double towardsUpper = (x - nodes[lower]) / width;
    const double towardsLower = 1.0 - towardsUpper;
    return towardsLower * ordinates[lower] + towardsUpper * ordinates[lower + 1] +
           ((towardsLower * towardsLower * towardsLower - towardsLower) * curvatures[lower] +
            (towardsUpper * towardsUpper * towardsUpper - towardsUpper) * curvatures[lower + 1]) *
               width * width / 6.0;
}

}
