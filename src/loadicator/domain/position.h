#pragma once

#include "loadicator/units/quantity.h"

namespace loadicator {

/// Ship coordinate system used throughout (the book's convention):
///   x forward from the aft perpendicular, y positive to port, z up from the baseline.
struct Position {
    Length x;
    Length y;
    Length z;
};

} // namespace loadicator
