#pragma once

#include "calc/analysis.h"

#include <ostream>

namespace loadicator {

void writeReport(std::ostream& output, const Analysis& analysis);

}
