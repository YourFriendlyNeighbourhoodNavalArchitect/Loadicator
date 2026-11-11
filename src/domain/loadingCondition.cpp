#include "domain/loadingCondition.h"

#include "util/error.h"

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

}
