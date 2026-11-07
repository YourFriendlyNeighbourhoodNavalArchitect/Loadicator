#pragma once

#include "loadicator/domain/compartmentTable.h"
#include "loadicator/domain/hydrostaticTable.h"
#include "loadicator/domain/loadingCondition.h"
#include "loadicator/domain/ship.h"
#include "loadicator/domain/shipParticulars.h"

#include <filesystem>

namespace loadicator::io {

/// Reads data/ship.csv, data/hydrostatics/*.csv and data/compartments/*.csv.
[[nodiscard]] Ship loadShip(const std::filesystem::path& dataDirectory);

[[nodiscard]] ShipParticulars loadShipParticulars(const std::filesystem::path& file);
[[nodiscard]] HydrostaticTable loadHydrostatics(const std::filesystem::path& evenKeelFile,
                                                const std::filesystem::path& trimGridFile);
[[nodiscard]] CompartmentTable loadCompartmentTable(const std::filesystem::path& file);
[[nodiscard]] CompartmentLibrary loadCompartments(const std::filesystem::path& directory);

/// Reads a loading-condition CSV (see data/README.md for the format).
[[nodiscard]] LoadingCondition loadLoadingCondition(const std::filesystem::path& file);

} // namespace loadicator::io
