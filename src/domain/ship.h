#pragma once

#include "domain/compartmentTable.h"
#include "domain/hydrostaticTable.h"
#include "util/quantity.h"

namespace loadicator {

struct ShipParticulars {
    Length lengthBetweenPerpendiculars;
    // From the aft perpendicular; the hydrostatic tables measure their draught here.
    Length referencePointX;
};

struct Ship {
    ShipParticulars particulars;
    HydrostaticTable hydrostatics;
    CompartmentLibrary compartments;
};

}
