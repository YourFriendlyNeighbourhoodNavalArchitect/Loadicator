#pragma once

#include <filesystem>
#include <string>

namespace loadicator::io {

[[nodiscard]] std::string readTextFile(const std::filesystem::path& path);

}
