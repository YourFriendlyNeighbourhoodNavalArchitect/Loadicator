#pragma once

#include "loadicator/domain/compartmentTable.h"
#include "loadicator/domain/weightItem.h"
#include "loadicator/units/quantity.h"

#include <string>
#include <variant>
#include <vector>

namespace loadicator {

/// Solid bulk cargo has no free surface; liquids do.
enum class CargoPhase { liquid, solid };

/// How much is in a tank: either its mass, or the filled fraction of its capacity.
using TankAmount = std::variant<Mass, Ratio>;

/// Contents of a compartment that has a capacity table.
struct TankLoad {
    CompartmentId compartment;
    TankAmount amount;
    Density density;
    CargoPhase phase = CargoPhase::liquid;
};

struct LoadingCondition {
    std::string name;
    std::vector<TankLoad> tankLoads;
    std::vector<WeightItem> fixedWeights; ///< lightship, stores, untabulated compartments
};

} // namespace loadicator
