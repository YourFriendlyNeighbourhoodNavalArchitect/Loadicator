#include "loadicator/numeric/interpolation.h"

#include "loadicator/util/error.h"

#include <algorithm>
#include <iterator>
#include <string>

namespace loadicator::numeric {

namespace {

constexpr double relativeRangeTolerance = 1e-9;

}

Abscissa::Abscissa(std::vector<double> nodes) : nodeValues(std::move(nodes)) {
    if (nodeValues.size() < 2) {
        throw LoadicatorError("interpolation needs at least two nodes, got " +
                              std::to_string(nodeValues.size()));
    }
    const auto unsorted = std::adjacent_find(nodeValues.begin(), nodeValues.end(),
                                             [](double current, double next) { return next <= current; });
    if (unsorted != nodeValues.end()) {
        throw LoadicatorError("interpolation nodes must be strictly increasing; node " +
                              std::to_string(std::distance(nodeValues.begin(), unsorted)) + " (" +
                              std::to_string(*unsorted) + ") is followed by " +
                              std::to_string(*(unsorted + 1)));
    }
}

Bracket Abscissa::locate(double x) const {
    const double tolerance = relativeRangeTolerance * (back() - front());
    if (x < front() - tolerance || x > back() + tolerance) {
        throw LoadicatorError("value " + std::to_string(x) + " is outside the tabulated range [" +
                              std::to_string(front()) + ", " + std::to_string(back()) + "]");
    }
    const double clamped = std::clamp(x, front(), back());
    const auto upper = std::upper_bound(nodeValues.begin() + 1, nodeValues.end() - 1, clamped);
    const auto lower = static_cast<std::size_t>(upper - nodeValues.begin()) - 1;
    return Bracket{lower, (clamped - nodeValues[lower]) / (nodeValues[lower + 1] - nodeValues[lower])};
}

double interpolate(std::span<const double> ordinates, Bracket bracket) {
    if (bracket.lower + 1 >= ordinates.size()) {
        throw LoadicatorError("interpolation bracket does not fit the ordinates");
    }
    const double lowerValue = ordinates[bracket.lower];
    return lowerValue + bracket.weight * (ordinates[bracket.lower + 1] - lowerValue);
}

}
