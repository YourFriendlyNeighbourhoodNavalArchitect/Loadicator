#pragma once

#include "loadicator/calc/massSummary.h"
#include "loadicator/domain/hydrostaticTable.h"
#include "loadicator/domain/shipParticulars.h"
#include "loadicator/util/quantity.h"

namespace loadicator {

// Trim is positive by the stern; the midship draught is at the reference point of the hydrostatic tables.
struct FloatingPosition {
    Length draughtMidships;
    Length draughtAft;
    Length draughtForward;
    Length trim;
    Length lcb;
    Length lcf;
    Length kmt;
    bool trimExtrapolated = false;
};

[[nodiscard]] FloatingPosition solveEquilibrium(const MassSummary& mass, const HydrostaticTable& hydrostatics,
                                                const ShipParticulars& ship);

}
