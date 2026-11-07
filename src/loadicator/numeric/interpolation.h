#pragma once

#include <cstddef>
#include <span>
#include <vector>

namespace loadicator::numeric {

/// What to do when a lookup falls outside the tabulated range.
enum class OutOfRange { reject, clamp, extrapolate };

/// Position of x between two adjacent nodes: value = y[lower] + weight * (y[lower + 1] - y[lower]).
/// The weight lies in [0, 1] except when extrapolating.
struct Bracket {
    std::size_t lower = 0;
    double weight = 0.0;
};

/// Strictly increasing interpolation nodes. Lookups always use the two nodes that bracket x,
/// found by binary search.
class Abscissa {
public:
    /// Throws LoadicatorError unless there are at least two nodes, strictly increasing.
    explicit Abscissa(std::vector<double> nodes);

    [[nodiscard]] Bracket locate(double x, OutOfRange policy = OutOfRange::reject) const;

    [[nodiscard]] std::span<const double> nodes() const { return nodeValues; }
    [[nodiscard]] std::size_t size() const { return nodeValues.size(); }
    [[nodiscard]] double front() const { return nodeValues.front(); }
    [[nodiscard]] double back() const { return nodeValues.back(); }

private:
    std::vector<double> nodeValues;
};

/// Evaluates ordinates (one per node) at a bracket from Abscissa::locate.
[[nodiscard]] double interpolate(std::span<const double> ordinates, Bracket bracket);

/// One-shot convenience for small tables.
[[nodiscard]] double interpolateLinear(std::span<const double> abscissae, std::span<const double> ordinates,
                                       double x, OutOfRange policy = OutOfRange::reject);

} // namespace loadicator::numeric
