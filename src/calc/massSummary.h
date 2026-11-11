#pragma once

#include "domain/position.h"
#include "domain/weightItem.h"
#include "util/quantity.h"

#include <span>

namespace loadicator {

struct MassSummary {
    Mass displacement;
    Position centreOfGravity;
    Moment freeSurfaceMoment;
};

[[nodiscard]] MassSummary summarizeWeights(std::span<const WeightItem> items);

}
