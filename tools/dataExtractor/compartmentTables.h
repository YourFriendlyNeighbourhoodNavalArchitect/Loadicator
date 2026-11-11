#pragma once

#include "shipData.h"
#include "sourceText.h"

#include <vector>

namespace loadicator::extractor {

[[nodiscard]] std::vector<CompartmentData> extractSoundingTables(const SourceText& source);

[[nodiscard]] CompartmentData extractHoldTable(const SourceText& source);

}
