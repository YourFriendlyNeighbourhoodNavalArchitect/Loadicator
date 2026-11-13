#include "domain/compartmentTable.h"

#include "util/error.h"

#include <algorithm>
#include <format>

namespace loadicator {

namespace {

// The masses of the book can place a full tank slightly above its tabulated capacity, by up to 0.13 %
// (R4.3: 26.5 t at 0.85 t/m3 is 31.18 m3 in 31.14 m3).
constexpr double capacityTolerance = 0.005;

}

CompartmentTable::CompartmentTable(const std::vector<Row>& rows)
    : volumes(magnitudes(rows, &Row::volume)),
      lcg(magnitudes(rows, [](const Row& row) { return row.centroid.x; })),
      tcg(magnitudes(rows, [](const Row& row) { return row.centroid.y; })),
      vcg(magnitudes(rows, [](const Row& row) { return row.centroid.z; })),
      inertia(magnitudes(rows, &Row::freeSurfaceInertia)) {}

CompartmentTable::Row CompartmentTable::stateAt(Volume volume) const {
    // An empty compartment has no free surface. It is placed at the lowest tabulated centroid, where its
    // zero mass has no effect.
    if (volume == Volume{0.0}) {
        return Row{.volume = volume, .centroid = centroidAt(numeric::Bracket{})};
    }
    if (volume < Volume{volumes.front()} || volume > capacity() * (1.0 + capacityTolerance)) {
        throw LoadicatorError(std::format("a volume of {:.2f} m3 is outside the table ({:.2f} .. {:.2f} m3)",
                                          volume.value(), volumes.front(), volumes.back()));
    }
    // Within the tolerance above the capacity, the compartment is full, and its contents keep their volume
    // so that their mass is the one given.
    const auto bracket = volumes.locate(std::min(volume, capacity()).value());
    return Row{.volume = volume,
               .centroid = centroidAt(bracket),
               .freeSurfaceInertia = SecondMomentOfArea{numeric::interpolate(inertia, bracket)}};
}

Position CompartmentTable::centroidAt(numeric::Bracket bracket) const {
    return Position{Length{numeric::interpolate(lcg, bracket)}, Length{numeric::interpolate(tcg, bracket)},
                    Length{numeric::interpolate(vcg, bracket)}};
}

}
