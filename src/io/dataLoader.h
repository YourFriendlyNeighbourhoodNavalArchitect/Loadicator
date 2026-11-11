#pragma once

#include "domain/loadingCondition.h"
#include "domain/ship.h"

#include <filesystem>

namespace loadicator::io {

[[nodiscard]] Ship loadShip(const std::filesystem::path& dataDirectory);

[[nodiscard]] LoadingCondition loadLoadingCondition(const std::filesystem::path& file);

}
