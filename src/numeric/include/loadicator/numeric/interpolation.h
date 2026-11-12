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

[[nodiscard]] double interpolate(std::span<const double> ordinates, Bracket bracket);

}
