#pragma once

#include "loadicator/numeric/interpolation.h"

#include <vector>

namespace loadicator::numeric {

class NaturalCubicSpline {
public:
    NaturalCubicSpline(Abscissa nodes, std::vector<double> ordinates);

    // Fails outside the nodes, beyond the rounding tolerance of Abscissa::locate: the spline does not
    // extrapolate.
    [[nodiscard]] double valueAt(double x) const;
    // The bracket must have been located on the nodes of this spline.
    [[nodiscard]] double valueAt(Bracket bracket) const;

private:
    Abscissa abscissa;
    std::vector<double> ordinates;
    std::vector<double> curvatures;
};

}
