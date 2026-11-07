#pragma once

#include "loadicator/calc/massSummary.h"
#include "loadicator/domain/hydrostaticTable.h"
#include "loadicator/domain/shipParticulars.h"
#include "loadicator/units/quantity.h"

#include <string_view>

namespace loadicator {

enum class EquilibriumMethod {
    /// Even-keel hydrostatics at the displacement, trim from MCT, rotation about LCF.
    firstOrder,
    /// Draught and trim solved together on the draught × trim grid, with B and G on one
    /// vertical in the trimmed attitude (default).
    freeTrim,
};

[[nodiscard]] std::string_view toString(EquilibriumMethod method);

/// Upright floating position. Trim is positive by the stern; draughts are moulded.
struct FloatingPosition {
    EquilibriumMethod method = EquilibriumMethod::freeTrim;
    Length draughtMidships; ///< at the hydrostatic reference point
    Length draughtAft;      ///< at the aft perpendicular
    Length draughtForward;  ///< at the forward perpendicular
    Length trim;
    Length lcb;
    Length lcf;
    Length kmt;
    /// True when the trim lies outside the tabulated trim range (values extrapolated).
    bool trimExtrapolated = false;
};

/// Throws if the displacement lies outside the hydrostatic tables or no equilibrium is found.
[[nodiscard]] FloatingPosition solveEquilibrium(const MassSummary& mass, const HydrostaticTable& hydrostatics,
                                                const ShipParticulars& ship, EquilibriumMethod method);

} // namespace loadicator
