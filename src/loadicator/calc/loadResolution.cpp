#include "loadicator/calc/loadResolution.h"

#include <variant>

namespace loadicator {

namespace {

[[nodiscard]] Volume volumeOf(Mass mass, Density density, Volume) {
    return mass / density;
}

[[nodiscard]] Volume volumeOf(Ratio fill, Density, Volume capacity) {
    return fill * capacity;
}

[[nodiscard]] WeightItem resolveTank(const TankLoad& load, const CompartmentLibrary& compartments) {
    const CompartmentTable& table = compartments.at(load.compartment);
    const Volume volume = std::visit(
        [&](auto amount) { return volumeOf(amount, load.density, table.capacity()); }, load.amount);
    const CompartmentState state = table.stateAt(volume);
    const Moment freeSurfaceMoment =
        load.phase == CargoPhase::liquid ? load.density * state.freeSurfaceInertia : Moment{0.0};
    return WeightItem{.name = load.compartment.value(),
                      .mass = load.density * state.volume,
                      .centre = state.centroid,
                      .freeSurfaceMoment = freeSurfaceMoment};
}

}

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

}
