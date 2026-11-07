#include "loadicator/calc/analysis.h"

#include "loadicator/calc/loadResolution.h"

#include <utility>

namespace loadicator {

Analysis analyzeLoadingCondition(const Ship& ship, const LoadingCondition& condition,
                                 EquilibriumMethod method) {
    std::vector<WeightItem> weights = resolveLoadingCondition(condition, ship.compartments);
    const MassSummary mass = summarizeWeights(weights);
    const FloatingPosition floatingPosition =
        solveEquilibrium(mass, ship.hydrostatics, ship.particulars, method);
    return Analysis{.conditionName = condition.name,
                    .weights = std::move(weights),
                    .mass = mass,
                    .floatingPosition = floatingPosition,
                    .stability = evaluateInitialStability(mass, floatingPosition.kmt)};
}

} // namespace loadicator
