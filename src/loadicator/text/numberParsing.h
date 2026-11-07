#pragma once

#include <optional>
#include <string_view>
#include <vector>

namespace loadicator::text {

/// Removes leading and trailing whitespace (space, tab, CR, LF, FF, VT).
[[nodiscard]] std::string_view trim(std::string_view text);

/// Parses a whole token as a decimal number: optional sign, digits, optional fraction.
/// Accepts the forms found in the source tables: "12", "-3.5", ".911", "-.116", "7.".
/// Returns nullopt for anything else, including empty or partially numeric tokens.
[[nodiscard]] std::optional<double> parseNumber(std::string_view token);

/// Extracts every decimal number from free text, left to right. A sign binds to the number
/// it precedes, so glued values such as "228.29-14.13" yield {228.29, -14.13}.
[[nodiscard]] std::vector<double> parseNumbers(std::string_view text);

/// Splits on a delimiter, keeping empty fields.
[[nodiscard]] std::vector<std::string_view> split(std::string_view text, char delimiter);

} // namespace loadicator::text
