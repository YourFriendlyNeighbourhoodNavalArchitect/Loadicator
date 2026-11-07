#pragma once

#include "loadicator/domain/compartmentTable.h"
#include "loadicator/domain/loadingCondition.h"
#include "loadicator/domain/weightItem.h"

#include <vector>

namespace loadicator {

/// Turns every tank load into a weight item using its capacity table, then appends the fixed
/// weights. Throws for unknown compartments, non-positive densities and overfilled tanks.
[[nodiscard]] std::vector<WeightItem> resolveLoadingCondition(const LoadingCondition& condition,
                                                              const CompartmentLibrary& compartments);

} // namespace loadicator
