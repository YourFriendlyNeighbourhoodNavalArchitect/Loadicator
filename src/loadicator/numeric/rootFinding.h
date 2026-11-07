#pragma once

#include "loadicator/core/error.h"

#include <cmath>
#include <concepts>
#include <string>

namespace loadicator::numeric {

/// Finds x in [lower, upper] with f(x) = 0 using the Illinois variant of regula falsi.
/// Requires f(lower) and f(upper) to differ in sign; converges when the bracket is narrower
/// than tolerance or |f(x)| is exactly zero.
template <std::invocable<double> Function>
[[nodiscard]] double findRoot(Function&& function, double lower, double upper, double tolerance,
                              int maxIterations = 200) {
    double lowerValue = function(lower);
    double upperValue = function(upper);
    if (lowerValue == 0.0) {
        return lower;
    }
    if (upperValue == 0.0) {
        return upper;
    }
    if (std::signbit(lowerValue) == std::signbit(upperValue)) {
        throw LoadicatorError("no root in [" + std::to_string(lower) + ", " + std::to_string(upper) +
                              "]: function has the same sign at both ends");
    }

    int retainedSide = 0; // -1: lower end kept last step, +1: upper end kept last step
    for (int iteration = 0; iteration < maxIterations; ++iteration) {
        const double candidate = (lower * upperValue - upper * lowerValue) / (upperValue - lowerValue);
        const double candidateValue = function(candidate);
        if (candidateValue == 0.0 || std::abs(upper - lower) < tolerance) {
            return candidate;
        }
        if (std::signbit(candidateValue) == std::signbit(upperValue)) {
            upper = candidate;
            upperValue = candidateValue;
            if (retainedSide == -1) {
                lowerValue /= 2.0; // Illinois step: stop the stale end from stalling convergence
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

} // namespace loadicator::numeric
