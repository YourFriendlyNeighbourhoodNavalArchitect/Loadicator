#pragma once

#include "shipData.h"
#include "sourceText.h"

namespace loadicator::extractor {

[[nodiscard]] HydrostaticData extractHydrostatics(const SourceText& source);

}
