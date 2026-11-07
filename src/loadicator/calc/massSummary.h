#pragma once

#include "loadicator/domain/position.h"
#include "loadicator/domain/weightItem.h"
#include "loadicator/units/quantity.h"

#include <span>

namespace loadicator {

/// Total weight, centre of gravity (solid) and summed free-surface moment.
struct MassSummary {
    Mass displacement;
    Position centreOfGravity;
    Moment freeSurfaceMoment;
};

/// Throws if the total mass is not positive.
[[nodiscard]] MassSummary summarizeWeights(std::span<const WeightItem> items);

} // namespace loadicator
