#pragma once

#include "util/quantity.h"

namespace loadicator {

struct ShipParticulars {
    Length lengthBetweenPerpendiculars;
    // From the aft perpendicular; the hydrostatic tables measure their draught here.
    Length referencePointX;
};

}
