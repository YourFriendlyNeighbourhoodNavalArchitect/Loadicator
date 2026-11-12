#pragma once

#include "loadicator/calc/equilibrium.h"
#include "loadicator/calc/massSummary.h"
#include "loadicator/calc/stability.h"
#include "loadicator/domain/loadingCondition.h"
#include "loadicator/domain/ship.h"
#include "loadicator/domain/weightItem.h"

#include <string>
#include <vector>

namespace loadicator {

struct Analysis {
    std::string conditionName;
    std::vector<WeightItem> weights;
    MassSummary mass;
    FloatingPosition floatingPosition;
    InitialStability stability;
};

[[nodiscard]] Analysis analyzeLoadingCondition(const Ship& ship, const LoadingCondition& condition);

}
