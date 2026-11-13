#pragma once

#include <format>
#include <string>
#include <string_view>

// The files of the ship data directory and their columns, in the order in which the data extractor writes
// them.
namespace loadicator::io::schema {

inline constexpr std::string_view shipFile = "ship.csv";
inline constexpr std::string_view shipColumns[]{"parameter", "value"};

inline constexpr std::string_view evenKeelFile = "hydrostatics/evenKeel.csv";
inline constexpr std::string_view evenKeelColumns[]{"draught", "lcf", "vcb"};

inline constexpr std::string_view trimGridFile = "hydrostatics/trimGrid.csv";
inline constexpr std::string_view trimGridColumns[]{"draught", "trim", "displacement", "lcb", "kmt"};

inline constexpr std::string_view compartmentDirectory = "compartments";
inline constexpr std::string_view compartmentColumns[]{"volume", "lcg", "tcg", "vcg", "inertia"};

inline constexpr std::string_view conditionDirectory = "conditions";
inline constexpr std::string_view conditionColumns[]{"kind",  "id",  "mass", "fillPercent", "density",
                                                     "phase", "lcg", "tcg",  "vcg",         "fsm"};

// The results that the trim and stability book prints for its loading conditions.
inline constexpr std::string_view bookResultsFile = "conditions/expected.csv";

// The name of a loading condition of the book, which is also the stem of its file.
[[nodiscard]] inline std::string bookConditionKey(int number) {
    return std::format("cond{:02}", number);
}

}
