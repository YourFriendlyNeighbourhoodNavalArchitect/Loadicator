#pragma once

#include "calc/equilibrium.h"
#include "domain/loadingCondition.h"
#include "domain/ship.h"
#include "util/quantity.h"

#include <optional>
#include <span>
#include <string>
#include <vector>

namespace loadicator {

struct MassSummary {
    Mass displacement;
    Position centreOfGravity;
    Moment freeSurfaceMoment;
};

struct InitialStability {
    Length gmSolid;
    Length freeSurfaceCorrection;
    Length gmFluid;
    // In degrees, positive to port; empty when GM fluid is not positive, where the small-angle formula has no
    // meaning.
    std::optional<double> heel;
};

struct Analysis {
    std::string conditionName;
    std::vector<WeightItem> weights;
    MassSummary mass;
    FloatingPosition floatingPosition;
    InitialStability stability;
};

// The contents of each compartment as a weight at its centroid, followed by the further weights. Fails if a
// compartment is loaded twice.
[[nodiscard]] std::vector<WeightItem> resolveLoadingCondition(const LoadingCondition& condition,
                                                              const CompartmentLibrary& compartments);

[[nodiscard]] MassSummary summarizeWeights(std::span<const WeightItem> items);

[[nodiscard]] Analysis analyzeLoadingCondition(const Ship& ship, const LoadingCondition& condition);

}
