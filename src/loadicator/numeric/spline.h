#pragma once

#include "loadicator/numeric/interpolation.h"

#include <vector>

namespace loadicator::numeric {

class NaturalCubicSpline {
public:
    NaturalCubicSpline(Abscissa nodes, std::vector<double> ordinates);

    // Fails outside the nodes: the spline does not extrapolate.
    [[nodiscard]] double valueAt(double x) const;

private:
    Abscissa abscissa;
    std::vector<double> ordinates;
    std::vector<double> curvatures;
};

}
