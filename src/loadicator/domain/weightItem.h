#pragma once

#include "loadicator/domain/position.h"
#include "loadicator/util/quantity.h"

#include <string>

namespace loadicator {

struct WeightItem {
    std::string name;
    Mass mass;
    Position centre;
    Moment freeSurfaceMoment;
};

}
