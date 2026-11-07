#include "loadicator/calc/loadResolution.h"

#include "loadicator/core/error.h"

#include <type_traits>
#include <variant>

namespace loadicator {

namespace {

[[nodiscard]] Volume volumeOf(const TankLoad& load, const CompartmentTable& table) {
    return std::visit(
        [&](auto amount) -> Volume {
            if constexpr (std::is_same_v<decltype(amount), Mass>) {
                return amount / load.density;
            } else {
                return amount * table.capacity();
            }
        },
        load.amount);
}

[[nodiscard]] WeightItem resolveTank(const TankLoad& load, const CompartmentLibrary& compartments) {
    if (load.density.value() <= 0.0) {
        throw LoadicatorError("compartment " + load.compartment.value() + ": density must be positive");
    }
    const CompartmentTable& table = compartments.at(load.compartment);
    const CompartmentState state = table.stateAt(volumeOf(load, table));
    const Moment freeSurfaceMoment =
        load.phase == CargoPhase::liquid ? load.density * state.freeSurfaceInertia : Moment{0.0};
    return WeightItem{.name = load.compartment.value(),
                      .mass = load.density * state.volume,
                      .centre = state.centroid,
                      .freeSurfaceMoment = freeSurfaceMoment};
}

} // namespace

std::vector<WeightItem> resolveLoadingCondition(const LoadingCondition& condition,
                                                const CompartmentLibrary& compartments) {
    std::vector<WeightItem> items;
    items.reserve(condition.tankLoads.size() + condition.fixedWeights.size());
    for (const TankLoad& load : condition.tankLoads) {
        items.push_back(resolveTank(load, compartments));
    }
    items.insert(items.end(), condition.fixedWeights.begin(), condition.fixedWeights.end());
    return items;
}

} // namespace loadicator
