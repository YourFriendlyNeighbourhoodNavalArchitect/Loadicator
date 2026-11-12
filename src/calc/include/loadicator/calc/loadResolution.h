#pragma once

#include "loadicator/domain/compartmentTable.h"
#include "loadicator/domain/loadingCondition.h"
#include "loadicator/domain/weightItem.h"

#include <vector>

namespace loadicator {

[[nodiscard]] std::vector<WeightItem> resolveLoadingCondition(const LoadingCondition& condition,
                                                              const CompartmentLibrary& compartments);

}
