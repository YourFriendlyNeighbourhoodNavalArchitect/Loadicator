#pragma once

#include "domain/compartmentTable.h"
#include "domain/weightItem.h"
#include "util/quantity.h"

#include <string>
#include <variant>
#include <vector>

namespace loadicator {

enum class CargoPhase { liquid, solid };

using TankAmount = std::variant<Mass, Ratio>;

struct TankLoad {
    TankLoad(CompartmentId tank, TankAmount contents, Density contentsDensity, CargoPhase contentsPhase);

    CompartmentId compartment;
    TankAmount amount;
    Density density;
    CargoPhase phase;
};

struct LoadingCondition {
    std::string name;
    std::vector<TankLoad> tankLoads;
    std::vector<WeightItem> fixedWeights;
};

}
