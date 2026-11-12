#include "loadicator/domain/compartmentTable.h"

#include "loadicator/util/error.h"
#include "loadicator/util/magnitudes.h"

#include <algorithm>
#include <format>
#include <string_view>
#include <utility>

namespace loadicator {

namespace {

// Masses are rounded to 0.1 t, which can place a full small tank slightly above its capacity
// (R4.3: 26.5 t at 0.85 t/m3 is 31.18 m3 in 31.14 m3).
constexpr double capacityTolerance = 0.005;

}

CompartmentId::CompartmentId(std::string id) : identifier(std::move(id)) {
    if (identifier.empty()) {
        throw LoadicatorError("compartment id may not be empty");
    }
}

CompartmentTable::CompartmentTable(CompartmentId id, const std::vector<Row>& rows)
    : compartmentId(std::move(id)),
      volumes(withContext("compartment " + compartmentId.value(),
                          [&] { return numeric::Abscissa(magnitudes(rows, &Row::volume)); })),
      lcg(magnitudes(rows, [](const Row& row) { return row.centroid.x; })),
      tcg(magnitudes(rows, [](const Row& row) { return row.centroid.y; })),
      vcg(magnitudes(rows, [](const Row& row) { return row.centroid.z; })),
      inertia(magnitudes(rows, &Row::freeSurfaceInertia)) {
}

CompartmentState CompartmentTable::stateAt(Volume volume) const {
    const auto outside = [&](std::string_view reason) {
        return LoadicatorError(std::format("compartment {}: a volume of {:.2f} m3 {}", compartmentId.value(),
                                           volume.value(), reason));
    };
    if (volume < Volume{0.0}) {
        throw outside("is negative");
    }
    // An empty compartment has no free surface. It is placed at the lowest tabulated centroid, where its
    // zero mass has no effect.
    if (volume == Volume{0.0}) {
        return CompartmentState{.volume = volume,
                                .centroid = centroidAt(numeric::Bracket{}),
                                .freeSurfaceInertia = SecondMomentOfArea{0.0}};
    }
    if (volume < Volume{volumes.front()}) {
        throw outside(
            std::format("lies below the lowest sounding of the table ({:.2f} m3)", volumes.front()));
    }
    if (volume > capacity() * (1.0 + capacityTolerance)) {
        throw outside(std::format("exceeds the capacity of {:.2f} m3", capacity().value()));
    }
    // Within the tolerance above the capacity, the compartment is full, and its contents keep their volume
    // so that their mass is the one given.
    const auto bracket = volumes.locate(std::min(volume, capacity()).value());
    return CompartmentState{.volume = volume,
                            .centroid = centroidAt(bracket),
                            .freeSurfaceInertia = SecondMomentOfArea{numeric::interpolate(inertia, bracket)}};
}

Position CompartmentTable::centroidAt(numeric::Bracket bracket) const {
    return Position{Length{numeric::interpolate(lcg, bracket)}, Length{numeric::interpolate(tcg, bracket)},
                    Length{numeric::interpolate(vcg, bracket)}};
}

void CompartmentLibrary::add(CompartmentTable table) {
    const CompartmentId id = table.id();
    if (!tables.emplace(id, std::move(table)).second) {
        throw LoadicatorError("duplicate compartment table " + id.value());
    }
}

const CompartmentTable& CompartmentLibrary::at(const CompartmentId& id) const {
    const auto found = tables.find(id);
    if (found == tables.end()) {
        throw LoadicatorError("no capacity table for compartment " + id.value());
    }
    return found->second;
}

}
