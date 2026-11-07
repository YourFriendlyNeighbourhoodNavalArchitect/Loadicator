#pragma once

#include "loadicator/domain/position.h"
#include "loadicator/units/quantity.h"

#include <string>

namespace loadicator {

/// A mass with its centre and free-surface moment (zero for solids and fixed items).
struct WeightItem {
    std::string name;
    Mass mass;
    Position centre;
    Moment freeSurfaceMoment;
};

} // namespace loadicator
