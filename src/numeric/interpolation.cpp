#include "numeric/interpolation.h"

#include "util/error.h"

#include <algorithm>
#include <format>
#include <utility>

namespace loadicator::numeric {

Abscissa::Abscissa(std::vector<double> nodes) : nodeValues(std::move(nodes)) {
    if (nodeValues.size() < 2) {
        throw LoadicatorError("interpolation needs at least two nodes");
    }
    const auto unsorted = std::adjacent_find(nodeValues.begin(), nodeValues.end(), std::greater_equal<>());
    if (unsorted != nodeValues.end()) {
        throw LoadicatorError(
            std::format("interpolation nodes must be strictly increasing, but {} is followed by {}",
                        *unsorted, *(unsorted + 1)));
    }
}

Bracket Abscissa::locate(double x) const {
    const double tolerance = 1e-9 * (back() - front());
    if (x < front() - tolerance || x > back() + tolerance) {
        throw LoadicatorError(std::format("{} is outside the tabulated range [{}, {}]", x, front(), back()));
    }
    const double clamped = std::clamp(x, front(), back());
    const auto upper = std::upper_bound(nodeValues.begin() + 1, nodeValues.end() - 1, clamped);
    const auto lower = static_cast<std::size_t>(upper - nodeValues.begin()) - 1;
    return Bracket{lower, (clamped - nodeValues[lower]) / (nodeValues[lower + 1] - nodeValues[lower])};
}

double interpolate(std::span<const double> ordinates, Bracket bracket) {
    const double lowerValue = ordinates[bracket.lower];
    return lowerValue + bracket.weight * (ordinates[bracket.lower + 1] - lowerValue);
}

NaturalCubicSpline::NaturalCubicSpline(Abscissa nodes, std::vector<double> values)
    : abscissa(std::move(nodes)), ordinates(std::move(values)), curvatures(abscissa.size(), 0.0) {
    // The curvatures at the inner nodes solve a tridiagonal system, by the Thomas algorithm; those at the
    // ends are zero.
    const auto x = abscissa.nodes();
    const std::size_t count = x.size();
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

double NaturalCubicSpline::valueAt(Bracket bracket) const {
    const std::size_t lower = bracket.lower;
    const double width = abscissa.nodes()[lower + 1] - abscissa.nodes()[lower];
    const double towardsUpper = bracket.weight;
    const double towardsLower = 1.0 - towardsUpper;
    return towardsLower * ordinates[lower] + towardsUpper * ordinates[lower + 1] +
           ((towardsLower * towardsLower * towardsLower - towardsLower) * curvatures[lower] +
            (towardsUpper * towardsUpper * towardsUpper - towardsUpper) * curvatures[lower + 1]) *
               width * width / 6.0;
}

SplineSurface::SplineSurface(Abscissa x, Abscissa y, const std::vector<double>& values)
    : xNodes(std::move(x)), yNodes(std::move(y)) {
    const std::size_t xCount = xNodes.size();
    for (std::size_t j = 0; j < yNodes.size(); ++j) {
        const auto first = values.begin() + static_cast<std::ptrdiff_t>(j * xCount);
        alongX.emplace_back(xNodes, std::vector<double>(first, first + static_cast<std::ptrdiff_t>(xCount)));

        std::vector<double> unitAtNode(yNodes.size(), 0.0);
        unitAtNode[j] = 1.0;
        yBasis.emplace_back(yNodes, std::move(unitAtNode));
    }
}

double SplineSurface::valueAt(double x, double y) const {
    const Bracket alongXBracket = xNodes.locate(x);
    const Bracket alongYBracket = yNodes.locate(y);
    double value = 0.0;
    for (std::size_t j = 0; j < alongX.size(); ++j) {
        value += yBasis[j].valueAt(alongYBracket) * alongX[j].valueAt(alongXBracket);
    }
    return value;
}

}
