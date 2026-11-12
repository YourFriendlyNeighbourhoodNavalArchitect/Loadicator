#include "loadicator/numeric/splineSurface.h"

#include "loadicator/util/error.h"

#include <cstddef>
#include <iterator>
#include <string>
#include <utility>

namespace loadicator::numeric {

SplineSurface::SplineSurface(Abscissa x, Abscissa y, const std::vector<double>& values)
    : xNodes(std::move(x)), yNodes(std::move(y)) {
    const std::size_t xCount = xNodes.size();
    const std::size_t yCount = yNodes.size();
    if (values.size() != xCount * yCount) {
        throw LoadicatorError("a spline surface on " + std::to_string(xCount) + " by " +
                              std::to_string(yCount) + " nodes needs " + std::to_string(xCount * yCount) +
                              " values, got " + std::to_string(values.size()));
    }
    for (std::size_t j = 0; j < yCount; ++j) {
        const auto first = values.begin() + static_cast<std::ptrdiff_t>(j * xCount);
        alongX.emplace_back(xNodes, std::vector<double>(first, first + static_cast<std::ptrdiff_t>(xCount)));

        std::vector<double> unitAtNode(yCount, 0.0);
        unitAtNode[j] = 1.0;
        yBasis.emplace_back(yNodes, std::move(unitAtNode));
    }
}

double SplineSurface::valueAt(double x, double y) const {
    const Bracket alongXBracket = xNodes.locate(x);
    const Bracket alongYBracket = yNodes.locate(y);
    double value = 0.0;
    for (std::size_t j = 0; j < alongX.size(); ++j) {
        value += yBasis[j].valueAt(alongYBracket) * alongX[j].valueAt(alongXBracket);
    }
    return value;
}

}
