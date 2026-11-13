#pragma once

#include "domain/ship.h"
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
};

// The draught and trim at which the ship floats with B and G on one vertical. The tables are never
// extrapolated: fails if there is no such equilibrium within them.
[[nodiscard]] FloatingPosition solveEquilibrium(Mass displacement, const Position& centreOfGravity,
                                                const HydrostaticTable& hydrostatics,
                                                const ShipParticulars& ship);

}
