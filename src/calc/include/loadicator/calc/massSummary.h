#pragma once

#include "loadicator/domain/position.h"
#include "loadicator/domain/weightItem.h"
#include "loadicator/util/quantity.h"

#include <span>

namespace loadicator {

struct MassSummary {
    Mass displacement;
    Position centreOfGravity;
    Moment freeSurfaceMoment;
};

[[nodiscard]] MassSummary summarizeWeights(std::span<const WeightItem> items);

}
