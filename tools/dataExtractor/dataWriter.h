#pragma once

#include "shipData.h"

#include <filesystem>

namespace loadicator::extractor {

void writeShipData(const ShipData& data, const std::filesystem::path& dataDirectory);

}
