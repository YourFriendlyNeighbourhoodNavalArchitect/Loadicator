#pragma once

#include "loadicator/units/quantity.h"

namespace loadicator {

struct ShipParticulars {
    Length lengthBetweenPerpendiculars;
    Length breadthMoulded;
    Length depthMoulded;
    Length designDraught;
    /// Longitudinal position of the hydrostatic reference point (RFP), from AP. Draughts in the
    /// hydrostatic tables are measured at this point.
    Length referencePointX;
};

} // namespace loadicator
