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

// The contents of a compartment: a mass that is not negative or a fill within 0 .. 100 %, of a positive
// density.
class TankLoad {
public:
    TankLoad(CompartmentId compartment, TankAmount amount, Density density, CargoPhase phase);

    [[nodiscard]] const CompartmentId& compartment() const { return compartmentId; }
    [[nodiscard]] const TankAmount& amount() const { return tankAmount; }
    [[nodiscard]] Density density() const { return contentsDensity; }
    [[nodiscard]] CargoPhase phase() const { return contentsPhase; }

private:
    CompartmentId compartmentId;
    TankAmount tankAmount;
    Density contentsDensity;
    CargoPhase contentsPhase;
};

// The contents of the compartments, each loaded at most once, and any further weights, each named.
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
