#include "loadicator/domain/loadingCondition.h"

#include "loadicator/util/error.h"

#include <algorithm>
#include <utility>

namespace loadicator {

namespace {

[[nodiscard]] bool isValid(Mass mass) {
    return mass >= Mass{0.0};
}

[[nodiscard]] bool isValid(Ratio fill) {
    return fill >= Ratio{0.0} && fill <= Ratio{1.0};
}

}

TankLoad::TankLoad(CompartmentId tank, TankAmount contents, Density contentsDensity, CargoPhase contentsPhase)
    : compartment(std::move(tank)), amount(contents), density(contentsDensity), phase(contentsPhase) {
    if (density <= Density{0.0}) {
        throw LoadicatorError("compartment " + compartment.value() + ": the density must be positive");
    }
    if (!std::visit([](auto value) { return isValid(value); }, amount)) {
        throw LoadicatorError("compartment " + compartment.value() +
                              ": the mass may not be negative and the fill must lie within 0 .. 100 %");
    }
}

LoadingCondition::LoadingCondition(std::string name) : conditionName(std::move(name)) {}

void LoadingCondition::addTankLoad(TankLoad load) {
    const auto sameCompartment = [&](const TankLoad& other) { return other.compartment == load.compartment; };
    if (std::any_of(tanks.begin(), tanks.end(), sameCompartment)) {
        throw LoadicatorError("compartment " + load.compartment.value() + " is loaded twice");
    }
    tanks.push_back(std::move(load));
}

void LoadingCondition::addFixedWeight(WeightItem weight) {
    weights.push_back(std::move(weight));
}

}
