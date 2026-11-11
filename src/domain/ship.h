#pragma once

#include "domain/compartmentTable.h"
#include "domain/hydrostaticTable.h"
#include "domain/shipParticulars.h"

namespace loadicator {

struct Ship {
    ShipParticulars particulars;
    HydrostaticTable hydrostatics;
    CompartmentLibrary compartments;
};

}
