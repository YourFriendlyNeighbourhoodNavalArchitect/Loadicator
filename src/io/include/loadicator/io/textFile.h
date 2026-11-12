#pragma once

#include <filesystem>
#include <string>

namespace loadicator::io {

// The content of the file, without a leading UTF-8 byte order mark.
[[nodiscard]] std::string readTextFile(const std::filesystem::path& path);

}
