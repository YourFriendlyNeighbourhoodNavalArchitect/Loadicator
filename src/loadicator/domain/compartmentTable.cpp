#include "loadicator/domain/compartmentTable.h"

#include "loadicator/util/error.h"
#include "loadicator/util/magnitudes.h"

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
    if (volume < Volume{0.0} || volume > capacity() * (1.0 + capacityTolerance)) {
        throw LoadicatorError("compartment " + compartmentId.value() + ": volume " +
                              std::to_string(volume.value()) + " m3 is outside [0, " +
                              std::to_string(capacity().value()) + "] m3");
    }
    const auto bracket = volumes.locate(volume.value(), numeric::OutOfRange::clamp);
    return CompartmentState{.volume = volume,
                            .centroid = Position{Length{numeric::interpolate(lcg, bracket)},
                                                 Length{numeric::interpolate(tcg, bracket)},
                                                 Length{numeric::interpolate(vcg, bracket)}},
                            .freeSurfaceInertia = SecondMomentOfArea{numeric::interpolate(inertia, bracket)}};
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
