#pragma once

#include "loadicator/calc/analysis.h"

#include <ostream>

namespace loadicator::report {

void writeReport(std::ostream& output, const Analysis& analysis);

}
