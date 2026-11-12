#pragma once

#include "loadicator/domain/compartmentTable.h"
#include "loadicator/domain/hydrostaticTable.h"
#include "loadicator/domain/shipParticulars.h"

namespace loadicator {

struct Ship {
    ShipParticulars particulars;
    HydrostaticTable hydrostatics;
    CompartmentLibrary compartments;
};

}
