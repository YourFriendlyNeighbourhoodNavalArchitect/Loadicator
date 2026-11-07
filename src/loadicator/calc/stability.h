#pragma once

#include "loadicator/calc/massSummary.h"
#include "loadicator/units/quantity.h"

#include <optional>

namespace loadicator {

/// Small-angle transverse stability.
struct InitialStability {
    Length kg;
    Length kmt;
    Length gmSolid;               ///< KMT − KG
    Length freeSurfaceCorrection; ///< ΣFSM / Δ
    Length gmFluid;               ///< GM solid − free-surface correction
    /// Static heel from the transverse centre of gravity, positive to port; empty when the ship
    /// is unstable (GM fluid ≤ 0), where the small-angle formula has no meaning.
    std::optional<Angle> heel;
};

[[nodiscard]] InitialStability evaluateInitialStability(const MassSummary& mass, Length kmt);

} // namespace loadicator
