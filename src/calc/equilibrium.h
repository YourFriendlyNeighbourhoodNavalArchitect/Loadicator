#pragma once

#include "calc/massSummary.h"
#include "domain/hydrostaticTable.h"
#include "domain/shipParticulars.h"
#include "util/quantity.h"

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
