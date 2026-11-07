#pragma once

#include "loadicator/calc/analysis.h"

#include <ostream>

namespace loadicator {

/// Plain-text report: weights table, floating position and initial stability.
void writeReport(std::ostream& output, const Analysis& analysis);

} // namespace loadicator
