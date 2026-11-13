#pragma once

#include <cstddef>
#include <span>
#include <vector>

namespace loadicator::numeric {

// The interval of an abscissa that holds a value: its lower node and the fraction of the way to the next.
struct Bracket {
    std::size_t lower = 0;
    double weight = 0.0;
};

// Strictly increasing nodes, at least two.
class Abscissa {
public:
    explicit Abscissa(std::vector<double> nodes);

    // Fails outside the nodes, beyond a tolerance for rounding within which x is taken to be the end node.
    [[nodiscard]] Bracket locate(double x) const;

    [[nodiscard]] std::span<const double> nodes() const { return nodeValues; }
    [[nodiscard]] std::size_t size() const { return nodeValues.size(); }
    [[nodiscard]] double front() const { return nodeValues.front(); }
    [[nodiscard]] double back() const { return nodeValues.back(); }

private:
    std::vector<double> nodeValues;
};

// Linear interpolation of ordinates given at the nodes of the abscissa on which the bracket was located.
[[nodiscard]] double interpolate(std::span<const double> ordinates, Bracket bracket);

// It does not extrapolate.
class NaturalCubicSpline {
public:
    NaturalCubicSpline(Abscissa nodes, std::vector<double> ordinates);

    [[nodiscard]] double valueAt(double x) const { return valueAt(abscissa.locate(x)); }
    // The bracket must have been located on the nodes of this spline.
    [[nodiscard]] double valueAt(Bracket bracket) const;

private:
    Abscissa abscissa;
    std::vector<double> ordinates;
    std::vector<double> curvatures;
};

// A function of two variables tabulated on a rectangular grid, interpolated by natural cubic splines in each
// variable. It does not extrapolate.
class SplineSurface {
public:
    // The values are given for each node of y in turn, at every node of x: values[j * x.size() + i] at
    // (x[i], y[j]).
    SplineSurface(Abscissa x, Abscissa y, const std::vector<double>& values);

    [[nodiscard]] double valueAt(double x, double y) const;

private:
    Abscissa xNodes;
    Abscissa yNodes;
    // One spline in x through the values at each node of y.
    std::vector<NaturalCubicSpline> alongX;
    // A natural cubic spline is linear in its ordinates, so the spline in y through the values of alongX is
    // the sum of those values, each weighted by the spline that is one at its own node of y and zero at the
    // others. These splines depend on the nodes of y alone and are built once.
    std::vector<NaturalCubicSpline> yBasis;
};

}
