#pragma once

#include "loadicator/numeric/interpolation.h"

#include <vector>

namespace loadicator::numeric {

/// Natural cubic spline through (nodes[i], ordinates[i]). Outside the nodes it continues
/// linearly with the end slope, which is better behaved than extending the cubic.
class NaturalCubicSpline {
public:
    /// Throws unless there are at least two strictly increasing nodes and one ordinate per node.
    NaturalCubicSpline(Abscissa nodes, std::vector<double> ordinates);

    [[nodiscard]] double valueAt(double x) const;

private:
    Abscissa abscissa;
    std::vector<double> ordinates;
    std::vector<double> curvatures; ///< second derivatives at the nodes (zero at both ends)
};

} // namespace loadicator::numeric
