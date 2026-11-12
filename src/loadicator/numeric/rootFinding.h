#pragma once

#include "loadicator/util/error.h"

#include <cmath>
#include <concepts>
#include <string>

namespace loadicator::numeric {

// An interval and the values of a function at its ends.
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

// Narrows an interval that brackets a root until it is shorter than the tolerance, by the Illinois variant of
// regula falsi. The function is evaluated only within the interval, and the result holds the values of the
// function at its ends, so that the caller can take the end on the side of the root it needs. At an exact
// root, both ends are the root.
template <std::invocable<double> Function>
[[nodiscard]] Interval narrowBracket(Function&& function, Interval interval, double tolerance,
                                     int maxIterations = 200) {
    if (!interval.bracketsRoot()) {
        throw LoadicatorError("no root in [" + std::to_string(interval.lower) + ", " +
                              std::to_string(interval.upper) + "]: function has the same sign at both ends");
    }
    const auto exactRoot = [](double root) { return Interval{root, root, 0.0, 0.0}; };
    if (interval.lowerValue == 0.0) {
        return exactRoot(interval.lower);
    }
    if (interval.upperValue == 0.0) {
        return exactRoot(interval.upper);
    }

    // The values at the ends as the secant sees them: the value at an end kept twice in a row is halved,
    // which keeps that end from stalling the convergence.
    double lowerWeight = interval.lowerValue;
    double upperWeight = interval.upperValue;
    int retainedSide = 0;
    for (int iteration = 0; iteration < maxIterations; ++iteration) {
        if (std::abs(interval.upper - interval.lower) < tolerance) {
            return interval;
        }
        double candidate =
            (interval.lower * upperWeight - interval.upper * lowerWeight) / (upperWeight - lowerWeight);
        // Rounding can place the secant on an end or just beyond it; bisection then keeps to the interval.
        if (!(candidate > interval.lower && candidate < interval.upper)) {
            candidate = (interval.lower + interval.upper) / 2.0;
        }
        const double candidateValue = function(candidate);
        if (candidateValue == 0.0) {
            return exactRoot(candidate);
        }
        if (std::signbit(candidateValue) == std::signbit(interval.upperValue)) {
            interval.upper = candidate;
            interval.upperValue = candidateValue;
            upperWeight = candidateValue;
            if (retainedSide == -1) {
                lowerWeight /= 2.0;
            }
            retainedSide = -1;
        } else {
            interval.lower = candidate;
            interval.lowerValue = candidateValue;
            lowerWeight = candidateValue;
            if (retainedSide == +1) {
                upperWeight /= 2.0;
            }
            retainedSide = +1;
        }
    }
    throw LoadicatorError("root finding did not converge in " + std::to_string(maxIterations) +
                          " iterations");
}

template <std::invocable<double> Function>
[[nodiscard]] double findRoot(Function&& function, Interval interval, double tolerance,
                              int maxIterations = 200) {
    const Interval narrowed = narrowBracket(function, interval, tolerance, maxIterations);
    return (narrowed.lower + narrowed.upper) / 2.0;
}

}
