#pragma once

#include "util/quantity.h"

#include <string>
#include <variant>
#include <vector>

namespace loadicator {

enum class CargoPhase { liquid, solid };

// The contents of a compartment, given by mass or by the filled fraction of its capacity. A liquid has a free
// surface and a solid has none.
struct TankLoad {
    std::string compartment;
    std::variant<Mass, Ratio> amount;
    Density density;
    CargoPhase phase = CargoPhase::liquid;
};

struct WeightItem {
    std::string name;
    Mass mass;
    Position centre;
    Moment freeSurfaceMoment;
};

struct LoadingCondition {
    std::string name;
    std::vector<TankLoad> tanks;
    std::vector<WeightItem> weights;
};

}
