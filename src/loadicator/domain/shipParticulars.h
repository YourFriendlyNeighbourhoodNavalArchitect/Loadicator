#pragma once

#include "loadicator/util/quantity.h"

namespace loadicator {

// The length between perpendiculars, which is positive, and the reference point of the hydrostatic tables,
// which lies between the perpendiculars.
class ShipParticulars {
public:
    ShipParticulars(Length lengthBetweenPerpendiculars, Length referencePointX);

    [[nodiscard]] Length lengthBetweenPerpendiculars() const { return length; }
    // From the aft perpendicular; the hydrostatic tables measure their draught here.
    [[nodiscard]] Length referencePointX() const { return referencePoint; }

private:
    Length length;
    Length referencePoint;
};

}
