#pragma once

#include "loadicator/numeric/interpolation.h"
#include "loadicator/numeric/spline.h"

#include <vector>

namespace loadicator::numeric {

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
