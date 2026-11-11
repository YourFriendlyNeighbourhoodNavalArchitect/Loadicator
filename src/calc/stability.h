#pragma once

#include "calc/massSummary.h"
#include "util/quantity.h"

#include <optional>

namespace loadicator {

struct InitialStability {
    Length gmSolid;
    Length freeSurfaceCorrection;
    Length gmFluid;
    // Positive to port; empty when GM fluid is not positive, where the small-angle formula has no meaning.
    std::optional<Angle> heel;
};

[[nodiscard]] InitialStability evaluateInitialStability(const MassSummary& mass, Length kmt);

}
