#include "loadicator/text/numberParsing.h"

#include <cstddef>
#include <cstdlib>
#include <string>

namespace loadicator::text {

namespace {

constexpr std::string_view whitespace = " \t\r\n\f\v";

[[nodiscard]] bool isDigit(char character) {
    return character >= '0' && character <= '9';
}

[[nodiscard]] bool isSign(char character) {
    return character == '-' || character == '+';
}

/// Length of the number starting at text[start], or 0 if no number starts there.
[[nodiscard]] std::size_t numberLengthAt(std::string_view text, std::size_t start) {
    std::size_t position = start;
    if (position < text.size() && isSign(text[position])) {
        ++position;
    }
    const std::size_t integerStart = position;
    while (position < text.size() && isDigit(text[position])) {
        ++position;
    }
    const bool hasIntegerDigits = position > integerStart;

    bool hasFractionDigits = false;
    if (position < text.size() && text[position] == '.') {
        std::size_t fractionEnd = position + 1;
        while (fractionEnd < text.size() && isDigit(text[fractionEnd])) {
            ++fractionEnd;
        }
        hasFractionDigits = fractionEnd > position + 1;
        if (hasIntegerDigits || hasFractionDigits) {
            position = fractionEnd;
        }
    }
    return (hasIntegerDigits || hasFractionDigits) ? position - start : 0;
}

[[nodiscard]] double toDouble(std::string_view token) {
    const std::string buffer{token};
    return std::strtod(buffer.c_str(), nullptr);
}

} // namespace

std::string_view trim(std::string_view text) {
    const std::size_t first = text.find_first_not_of(whitespace);
    if (first == std::string_view::npos) {
        return {};
    }
    const std::size_t last = text.find_last_not_of(whitespace);
    return text.substr(first, last - first + 1);
}

std::optional<double> parseNumber(std::string_view token) {
    const std::string_view trimmed = trim(token);
    if (trimmed.empty() || numberLengthAt(trimmed, 0) != trimmed.size()) {
        return std::nullopt;
    }
    return toDouble(trimmed);
}

std::vector<double> parseNumbers(std::string_view text) {
    std::vector<double> numbers;
    std::size_t position = 0;
    while (position < text.size()) {
        const std::size_t length = numberLengthAt(text, position);
        if (length == 0) {
            ++position;
            continue;
        }
        numbers.push_back(toDouble(text.substr(position, length)));
        position += length;
    }
    return numbers;
}

std::vector<std::string_view> split(std::string_view text, char delimiter) {
    std::vector<std::string_view> fields;
    std::size_t start = 0;
    while (true) {
        const std::size_t end = text.find(delimiter, start);
        fields.push_back(
            text.substr(start, end == std::string_view::npos ? std::string_view::npos : end - start));
        if (end == std::string_view::npos) {
            return fields;
        }
        start = end + 1;
    }
}

} // namespace loadicator::text
