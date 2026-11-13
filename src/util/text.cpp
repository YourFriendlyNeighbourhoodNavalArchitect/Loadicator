#include "util/text.h"

#include <charconv>
#include <cmath>

namespace loadicator::text {

std::string_view trim(std::string_view text) {
    constexpr std::string_view whitespace = " \t\r\n\f\v";
    const std::size_t first = text.find_first_not_of(whitespace);
    if (first == std::string_view::npos) {
        return {};
    }
    return text.substr(first, text.find_last_not_of(whitespace) - first + 1);
}

std::optional<double> parseNumber(std::string_view token) {
    token = trim(token);
    if (token.starts_with('+')) {
        token.remove_prefix(1);
    }
    double value = 0.0;
    const char* const end = token.data() + token.size();
    const auto [stop, error] = std::from_chars(token.data(), end, value);
    if (token.empty() || error != std::errc{} || stop != end || !std::isfinite(value)) {
        return std::nullopt;
    }
    return value;
}

std::vector<std::string_view> split(std::string_view text, char delimiter) {
    std::vector<std::string_view> fields;
    for (std::size_t start = 0;;) {
        const std::size_t end = text.find(delimiter, start);
        fields.push_back(text.substr(start, end - start));
        if (end == std::string_view::npos) {
            return fields;
        }
        start = end + 1;
    }
}

}
