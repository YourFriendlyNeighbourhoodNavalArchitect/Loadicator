#pragma once

#include "loadicator/domain/position.h"
#include "loadicator/numeric/interpolation.h"
#include "loadicator/util/quantity.h"

#include <compare>
#include <map>
#include <string>
#include <vector>

namespace loadicator {

class CompartmentId {
public:
    explicit CompartmentId(std::string id);

    [[nodiscard]] const std::string& value() const { return identifier; }

    friend auto operator<=>(const CompartmentId&, const CompartmentId&) = default;

private:
    std::string identifier;
};

struct CompartmentState {
    Volume volume;
    Position centroid;
    SecondMomentOfArea freeSurfaceInertia;
};

class CompartmentTable {
public:
    struct Row {
        Volume volume;
        Position centroid;
        SecondMomentOfArea freeSurfaceInertia;
    };

    CompartmentTable(CompartmentId id, const std::vector<Row>& rows);

    [[nodiscard]] const CompartmentId& id() const { return compartmentId; }
    [[nodiscard]] Volume capacity() const { return Volume{volumes.back()}; }

    [[nodiscard]] CompartmentState stateAt(Volume volume) const;

private:
    CompartmentId compartmentId;
    numeric::Abscissa volumes;
    std::vector<double> lcg;
    std::vector<double> tcg;
    std::vector<double> vcg;
    std::vector<double> inertia;
};

class CompartmentLibrary {
public:
    void add(CompartmentTable table);

    [[nodiscard]] const CompartmentTable& at(const CompartmentId& id) const;

private:
    std::map<CompartmentId, CompartmentTable> tables;
};

}
