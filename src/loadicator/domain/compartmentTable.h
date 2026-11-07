#pragma once

#include "loadicator/domain/position.h"
#include "loadicator/numeric/interpolation.h"
#include "loadicator/units/quantity.h"

#include <compare>
#include <map>
#include <string>
#include <vector>

namespace loadicator {

/// Book identifier of a compartment, e.g. "R2.02S" or "R1.6".
class CompartmentId {
public:
    explicit CompartmentId(std::string id);

    [[nodiscard]] const std::string& value() const { return identifier; }

    friend auto operator<=>(const CompartmentId&, const CompartmentId&) = default;

private:
    std::string identifier;
};

/// Contents geometry at a given volume.
struct CompartmentState {
    Volume volume;
    Position centroid;
    SecondMomentOfArea freeSurfaceInertia;
};

/// Capacity table of a tank or hold: centroid and free-surface inertia against volume.
class CompartmentTable {
public:
    struct Row {
        Volume volume;
        Position centroid;
        SecondMomentOfArea freeSurfaceInertia;
    };

    /// Throws unless volumes strictly increase and capacity is at least the largest volume.
    CompartmentTable(CompartmentId id, std::string description, Volume capacity,
                     const std::vector<Row>& rows);

    [[nodiscard]] const CompartmentId& id() const { return compartmentId; }
    [[nodiscard]] const std::string& description() const { return descriptionText; }
    /// Volume at 100 % fill; may slightly exceed the last tabulated volume.
    [[nodiscard]] Volume capacity() const { return fullCapacity; }

    /// Interpolated state; volumes between the table ends and [0, capacity] take the end values.
    /// Throws for negative volumes or volumes beyond capacity.
    [[nodiscard]] CompartmentState stateAt(Volume volume) const;

private:
    CompartmentId compartmentId;
    std::string descriptionText;
    Volume fullCapacity;
    numeric::Abscissa volumes;
    std::vector<double> lcg;
    std::vector<double> tcg;
    std::vector<double> vcg;
    std::vector<double> inertia;
};

/// All capacity tables of the ship, by id.
class CompartmentLibrary {
public:
    void add(CompartmentTable table);

    [[nodiscard]] const CompartmentTable& at(const CompartmentId& id) const;
    [[nodiscard]] bool contains(const CompartmentId& id) const { return tables.contains(id); }
    [[nodiscard]] std::size_t size() const { return tables.size(); }

private:
    std::map<CompartmentId, CompartmentTable> tables;
};

} // namespace loadicator
