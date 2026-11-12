#pragma once

#include <array>
#include <string_view>

// The results that the trim and stability book prints for each of its loading conditions. The data
// extractor writes them and the comparison with the book reads them; the program itself does not use them.
namespace loadicator::book {

// Relative to the ship data directory.
inline constexpr std::string_view resultsFile = "conditions/expected.csv";

namespace column {
inline constexpr std::string_view condition = "condition";
inline constexpr std::string_view displacement = "displacement";
inline constexpr std::string_view lcg = "lcg";
inline constexpr std::string_view tcg = "tcg";
inline constexpr std::string_view kg = "kg";
inline constexpr std::string_view draught = "draught";
inline constexpr std::string_view trim = "trim";
inline constexpr std::string_view heel = "heel";
inline constexpr std::string_view draughtAft = "draughtAft";
inline constexpr std::string_view draughtForward = "draughtForward";
inline constexpr std::string_view kmt = "kmt";
inline constexpr std::string_view gmSolid = "gmSolid";
inline constexpr std::string_view gm = "gm";
}

inline constexpr std::array columns{column::condition, column::displacement, column::lcg,  column::tcg,
                                    column::kg,        column::draught,      column::trim, column::heel,
                                    column::draughtAft, column::draughtForward, column::kmt, column::gmSolid,
                                    column::gm};

}
