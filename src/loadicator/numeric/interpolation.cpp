#include "loadicator/numeric/interpolation.h"

#include "loadicator/core/error.h"

#include <algorithm>
#include <iterator>
#include <string>

namespace loadicator::numeric {

namespace {

/// Values this close outside the range are treated as on the boundary (rounding noise).
constexpr double relativeRangeTolerance = 1e-9;

} // namespace

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

Bracket Abscissa::locate(double x, OutOfRange policy) const {
    const std::size_t lastInterval = nodeValues.size() - 2;
    const double tolerance = relativeRangeTolerance * (back() - front());

    const auto bracketAt = [this](std::size_t lower, double value) {
        return Bracket{lower, (value - nodeValues[lower]) / (nodeValues[lower + 1] - nodeValues[lower])};
    };

    if (x < front() - tolerance || x > back() + tolerance) {
        const bool below = x < front();
        switch (policy) {
        case OutOfRange::reject:
            throw LoadicatorError("value " + std::to_string(x) + " is outside the tabulated range [" +
                                  std::to_string(front()) + ", " + std::to_string(back()) + "]");
        case OutOfRange::clamp:
            return below ? Bracket{0, 0.0} : Bracket{lastInterval, 1.0};
        case OutOfRange::extrapolate:
            return bracketAt(below ? 0 : lastInterval, x);
        }
    }

    const double clamped = std::clamp(x, front(), back());
    const auto upper = std::upper_bound(nodeValues.begin(), nodeValues.end(), clamped);
    const auto lower =
        static_cast<std::size_t>(std::max<std::ptrdiff_t>(0, std::distance(nodeValues.begin(), upper) - 1));
    return bracketAt(std::min(lower, lastInterval), clamped);
}

double interpolate(std::span<const double> ordinates, Bracket bracket) {
    if (bracket.lower + 1 >= ordinates.size()) {
        throw LoadicatorError("interpolation bracket does not fit the ordinates");
    }
    const double lowerValue = ordinates[bracket.lower];
    return lowerValue + bracket.weight * (ordinates[bracket.lower + 1] - lowerValue);
}

double interpolateLinear(std::span<const double> abscissae, std::span<const double> ordinates, double x,
                         OutOfRange policy) {
    if (abscissae.size() != ordinates.size()) {
        throw LoadicatorError("interpolation needs as many ordinates as abscissae");
    }
    const Abscissa nodes{std::vector<double>(abscissae.begin(), abscissae.end())};
    return interpolate(ordinates, nodes.locate(x, policy));
}

} // namespace loadicator::numeric
