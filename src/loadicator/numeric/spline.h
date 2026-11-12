#pragma once

#include "loadicator/numeric/interpolation.h"

#include <vector>

namespace loadicator::numeric {

class NaturalCubicSpline {
public:
    NaturalCubicSpline(Abscissa nodes, std::vector<double> ordinates);

    [[nodiscard]] double valueAt(double x) const;

private:
    Abscissa abscissa;
    std::vector<double> ordinates;
    std::vector<double> curvatures;
};

}
