#pragma once

#include "loadicator/domain/compartmentTable.h"
#include "loadicator/domain/weightItem.h"
#include "loadicator/util/quantity.h"

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

// The contents of the compartments, each loaded at most once, and any further weights.
class LoadingCondition {
public:
    explicit LoadingCondition(std::string name);

    void addTankLoad(TankLoad load);
    void addFixedWeight(WeightItem weight);

    [[nodiscard]] const std::string& name() const { return conditionName; }
    [[nodiscard]] const std::vector<TankLoad>& tankLoads() const { return tanks; }
    [[nodiscard]] const std::vector<WeightItem>& fixedWeights() const { return weights; }

private:
    std::string conditionName;
    std::vector<TankLoad> tanks;
    std::vector<WeightItem> weights;
};

}
