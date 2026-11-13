#include "calc/analysis.h"

#include "util/error.h"

#include <cmath>
#include <numbers>
#include <set>
#include <utility>

namespace loadicator {

namespace {

[[nodiscard]] WeightItem resolveTank(const TankLoad& load, const CompartmentLibrary& compartments) {
    const auto table = compartments.find(load.compartment);
    if (table == compartments.end()) {
        throw LoadicatorError("no capacity table for compartment " + load.compartment);
    }
    return withContext("compartment " + load.compartment, [&] {
        if (load.density <= Density{0.0}) {
            throw LoadicatorError("the density must be positive");
        }
        const Volume volume = std::holds_alternative<Mass>(load.amount)
                                  ? std::get<Mass>(load.amount) / load.density
                                  : std::get<Ratio>(load.amount) * table->second.capacity();
        const CompartmentTable::Row state = table->second.stateAt(volume);
        return WeightItem{.name = load.compartment,
                          .mass = load.density * state.volume,
                          .centre = state.centroid,
                          .freeSurfaceMoment = load.phase == CargoPhase::liquid
                                                   ? load.density * state.freeSurfaceInertia
                                                   : Moment{0.0}};
    });
}

[[nodiscard]] InitialStability evaluateInitialStability(const MassSummary& mass, Length kmt) {
    const Length gmSolid = kmt - mass.centreOfGravity.z;
    const Length freeSurfaceCorrection = mass.freeSurfaceMoment / mass.displacement;
    const Length gmFluid = gmSolid - freeSurfaceCorrection;
    std::optional<double> heel;
    if (gmFluid > Length{0.0}) {
        heel = std::atan((mass.centreOfGravity.y / gmFluid).value()) * 180.0 / std::numbers::pi;
    }
    return InitialStability{
        .gmSolid = gmSolid, .freeSurfaceCorrection = freeSurfaceCorrection, .gmFluid = gmFluid, .heel = heel};
}

}

std::vector<WeightItem> resolveLoadingCondition(const LoadingCondition& condition,
                                                const CompartmentLibrary& compartments) {
    std::vector<WeightItem> items;
    std::set<std::string> loaded;
    for (const TankLoad& load : condition.tanks) {
        if (!loaded.insert(load.compartment).second) {
            throw LoadicatorError("compartment " + load.compartment + " is loaded twice");
        }
        items.push_back(resolveTank(load, compartments));
    }
    items.insert(items.end(), condition.weights.begin(), condition.weights.end());
    return items;
}

MassSummary summarizeWeights(std::span<const WeightItem> items) {
    Mass total{0.0};
    Moment longitudinal{0.0};
    Moment transverse{0.0};
    Moment vertical{0.0};
    Moment freeSurface{0.0};
    for (const WeightItem& item : items) {
        total += item.mass;
        longitudinal += item.mass * item.centre.x;
        transverse += item.mass * item.centre.y;
        vertical += item.mass * item.centre.z;
        freeSurface += item.freeSurfaceMoment;
    }
    if (total <= Mass{0.0}) {
        throw LoadicatorError("total displacement must be positive");
    }
    return MassSummary{.displacement = total,
                       .centreOfGravity =
                           Position{longitudinal / total, transverse / total, vertical / total},
                       .freeSurfaceMoment = freeSurface};
}

Analysis analyzeLoadingCondition(const Ship& ship, const LoadingCondition& condition) {
    std::vector<WeightItem> weights = resolveLoadingCondition(condition, ship.compartments);
    const MassSummary mass = summarizeWeights(weights);
    const FloatingPosition floatingPosition =
        solveEquilibrium(mass.displacement, mass.centreOfGravity, ship.hydrostatics, ship.particulars);
    return Analysis{.conditionName = condition.name,
                    .weights = std::move(weights),
                    .mass = mass,
                    .floatingPosition = floatingPosition,
                    .stability = evaluateInitialStability(mass, floatingPosition.kmt)};
}

}
