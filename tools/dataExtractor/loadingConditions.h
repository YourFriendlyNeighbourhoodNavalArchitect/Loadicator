#pragma once

#include "shipData.h"
#include "sourceText.h"

#include <set>
#include <string>
#include <vector>

namespace loadicator::extractor {

[[nodiscard]] std::vector<BookCondition>
extractLoadingConditions(const SourceText& source,
                         const std::set<std::string, std::less<>>& tabulatedCompartments);

}
