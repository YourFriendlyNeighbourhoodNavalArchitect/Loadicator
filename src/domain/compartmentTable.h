#pragma once

#include "numeric/interpolation.h"
#include "util/quantity.h"

#include <map>
#include <string>
#include <vector>

namespace loadicator {

// The capacity table of a compartment: the centroid and the free-surface inertia of its contents by volume.
class CompartmentTable {
public:
    struct Row {
        Volume volume;
        Position centroid;
        SecondMomentOfArea freeSurfaceInertia;
    };

    explicit CompartmentTable(const std::vector<Row>& rows);

    [[nodiscard]] Volume capacity() const { return Volume{volumes.back()}; }

    // Fails for a volume between empty and the lowest sounding of the table, which the table does not
    // describe, and for a volume beyond the capacity, save for the rounding of masses.
    [[nodiscard]] Row stateAt(Volume volume) const;

private:
    [[nodiscard]] Position centroidAt(numeric::Bracket bracket) const;

    numeric::Abscissa volumes;
    std::vector<double> lcg;
    std::vector<double> tcg;
    std::vector<double> vcg;
    std::vector<double> inertia;
};

// The capacity tables by compartment identifier.
using CompartmentLibrary = std::map<std::string, CompartmentTable, std::less<>>;

}
