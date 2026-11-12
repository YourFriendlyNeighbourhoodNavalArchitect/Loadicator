#pragma once

#include "loadicator/util/quantity.h"

namespace loadicator {

// x forward of the aft perpendicular, y positive to port, z above the baseline.
struct Position {
    Length x;
    Length y;
    Length z;
};

}
