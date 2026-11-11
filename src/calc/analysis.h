#pragma once

#include "calc/equilibrium.h"
#include "calc/massSummary.h"
#include "calc/stability.h"
#include "domain/loadingCondition.h"
#include "domain/ship.h"
#include "domain/weightItem.h"

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
