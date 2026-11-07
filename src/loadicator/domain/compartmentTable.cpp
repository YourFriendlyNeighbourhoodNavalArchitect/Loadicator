#include "loadicator/domain/compartmentTable.h"

#include "loadicator/core/error.h"

#include <utility>

namespace loadicator {

namespace {

/// Allowance for masses rounded to 0.1 t, which can place a full small tank (R4.3: 26.5 t at
/// 0.85 t/m3 = 31.18 m3 in 31.14 m3) slightly above capacity.
constexpr double capacityTolerance = 0.005;

[[nodiscard]] std::vector<double> volumesOf(const std::vector<CompartmentTable::Row>& rows) {
    std::vector<double> values;
    values.reserve(rows.size());
    for (const auto& row : rows) {
        values.push_back(row.volume.value());
    }
    return values;
}

template <typename Selector>
[[nodiscard]] std::vector<double> columnOf(const std::vector<CompartmentTable::Row>& rows, Selector select) {
    std::vector<double> values;
    values.reserve(rows.size());
    for (const auto& row : rows) {
        values.push_back(select(row).value());
    }
    return values;
}

} // namespace

CompartmentId::CompartmentId(std::string id) : identifier(std::move(id)) {
    if (identifier.empty()) {
        throw LoadicatorError("compartment id may not be empty");
    }
}

CompartmentTable::CompartmentTable(CompartmentId id, std::string description, Volume capacity,
                                   const std::vector<Row>& rows)
    : compartmentId(std::move(id)), descriptionText(std::move(description)), fullCapacity(capacity),
      volumes([&] {
          try {
              return numeric::Abscissa(volumesOf(rows));
          } catch (const LoadicatorError& error) {
              throw LoadicatorError("compartment " + compartmentId.value() + ": " + error.what());
          }
      }()),
      lcg(columnOf(rows, [](const Row& row) { return row.centroid.x; })),
      tcg(columnOf(rows, [](const Row& row) { return row.centroid.y; })),
      vcg(columnOf(rows, [](const Row& row) { return row.centroid.z; })),
      inertia(columnOf(rows, [](const Row& row) { return row.freeSurfaceInertia; })) {
    if (fullCapacity.value() < volumes.back()) {
        throw LoadicatorError("compartment " + compartmentId.value() +
                              ": capacity is below the largest volume");
    }
}

CompartmentState CompartmentTable::stateAt(Volume volume) const {
    if (volume.value() < 0.0 || volume.value() > fullCapacity.value() * (1.0 + capacityTolerance)) {
        throw LoadicatorError("compartment " + compartmentId.value() + ": volume " +
                              std::to_string(volume.value()) + " m3 is outside [0, " +
                              std::to_string(fullCapacity.value()) + "] m3");
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

} // namespace loadicator
