#pragma once

#include <optional>
#include <string_view>
#include <vector>

namespace loadicator::text {

[[nodiscard]] std::string_view trim(std::string_view text);

[[nodiscard]] std::optional<double> parseNumber(std::string_view token);

[[nodiscard]] std::vector<std::string_view> split(std::string_view text, char delimiter);

}
