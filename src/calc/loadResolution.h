#pragma once

#include "domain/compartmentTable.h"
#include "domain/loadingCondition.h"
#include "domain/weightItem.h"

#include <vector>

namespace loadicator {

[[nodiscard]] std::vector<WeightItem> resolveLoadingCondition(const LoadingCondition& condition,
                                                              const CompartmentLibrary& compartments);

}
