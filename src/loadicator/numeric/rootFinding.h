#pragma once

#include "loadicator/util/error.h"

#include <cmath>
#include <concepts>
#include <string>

namespace loadicator::numeric {

struct Interval {
    double lower = 0.0;
    double upper = 0.0;
    double lowerValue = 0.0;
    double upperValue = 0.0;

    [[nodiscard]] bool bracketsRoot() const {
        return lowerValue == 0.0 || upperValue == 0.0 || std::signbit(lowerValue) != std::signbit(upperValue);
    }
};

template <std::invocable<double> Function>
[[nodiscard]] Interval evaluate(Function&& function, double lower, double upper) {
    return Interval{lower, upper, function(lower), function(upper)};
}

template <std::invocable<double> Function>
[[nodiscard]] double findRoot(Function&& function, Interval interval, double tolerance,
                              int maxIterations = 200) {
    auto [lower, upper, lowerValue, upperValue] = interval;
    if (lowerValue == 0.0) {
        return lower;
    }
    if (upperValue == 0.0) {
        return upper;
    }
    if (!interval.bracketsRoot()) {
        throw LoadicatorError("no root in [" + std::to_string(lower) + ", " + std::to_string(upper) +
                              "]: function has the same sign at both ends");
    }

    int retainedSide = 0;
    for (int iteration = 0; iteration < maxIterations; ++iteration) {
        const double candidate = (lower * upperValue - upper * lowerValue) / (upperValue - lowerValue);
        const double candidateValue = function(candidate);
        if (candidateValue == 0.0) {
            return candidate;
        }
        if (std::signbit(candidateValue) == std::signbit(upperValue)) {
            upper = candidate;
            upperValue = candidateValue;
            if (retainedSide == -1) {
                // Illinois step: halving the value at the stale end keeps it from stalling convergence.
                lowerValue /= 2.0;
            }
            retainedSide = -1;
        } else {
            lower = candidate;
            lowerValue = candidateValue;
            if (retainedSide == +1) {
                upperValue /= 2.0;
            }
            retainedSide = +1;
        }
        if (std::abs(upper - lower) < tolerance) {
            return (lower + upper) / 2.0;
        }
    }
    throw LoadicatorError("root finding did not converge in " + std::to_string(maxIterations) +
                          " iterations");
}

}
