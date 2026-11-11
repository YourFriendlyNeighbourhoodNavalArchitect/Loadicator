#pragma once

#include "domain/position.h"
#include "util/quantity.h"

#include <string>

namespace loadicator {

struct WeightItem {
    std::string name;
    Mass mass;
    Position centre;
    Moment freeSurfaceMoment;
};

}
