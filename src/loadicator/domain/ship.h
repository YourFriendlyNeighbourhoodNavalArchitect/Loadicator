#pragma once

#include "loadicator/domain/compartmentTable.h"
#include "loadicator/domain/hydrostaticTable.h"
#include "loadicator/domain/shipParticulars.h"

namespace loadicator {

/// Everything known about the ship itself, independent of how it is loaded.
struct Ship {
    ShipParticulars particulars;
    HydrostaticTable hydrostatics;
    CompartmentLibrary compartments;
};

} // namespace loadicator
