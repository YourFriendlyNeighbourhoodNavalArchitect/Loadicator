#pragma once

#include "calc/analysis.h"

#include <ostream>

namespace loadicator::io {

void writeReport(std::ostream& output, const Analysis& analysis);

}
