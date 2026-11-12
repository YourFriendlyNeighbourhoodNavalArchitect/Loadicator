#pragma once

#include <array>
#include <format>
#include <string>
#include <string_view>

namespace loadicator::io::schema {

namespace files {
inline constexpr std::string_view ship = "ship.csv";
inline constexpr std::string_view evenKeel = "hydrostatics/evenKeel.csv";
inline constexpr std::string_view trimGrid = "hydrostatics/trimGrid.csv";
inline constexpr std::string_view compartments = "compartments";
inline constexpr std::string_view conditions = "conditions";
inline constexpr std::string_view extension = ".csv";
}

[[nodiscard]] inline std::string bookConditionKey(int number) {
    return std::format("cond{:02}", number);
}

namespace ship {
inline constexpr std::string_view parameter = "parameter";
inline constexpr std::string_view value = "value";
inline constexpr std::string_view lengthBetweenPerpendiculars = "lengthBetweenPerpendiculars";
inline constexpr std::string_view referencePointX = "referencePointX";
inline constexpr std::array columns{parameter, value};
}

namespace evenKeel {
inline constexpr std::string_view draught = "draught";
inline constexpr std::string_view lcf = "lcf";
inline constexpr std::string_view vcb = "vcb";
inline constexpr std::array columns{draught, lcf, vcb};
}

namespace trimGrid {
inline constexpr std::string_view draught = "draught";
inline constexpr std::string_view trim = "trim";
inline constexpr std::string_view displacement = "displacement";
inline constexpr std::string_view lcb = "lcb";
inline constexpr std::string_view kmt = "kmt";
inline constexpr std::array columns{draught, trim, displacement, lcb, kmt};
}

namespace compartment {
inline constexpr std::string_view volume = "volume";
inline constexpr std::string_view lcg = "lcg";
inline constexpr std::string_view tcg = "tcg";
inline constexpr std::string_view vcg = "vcg";
inline constexpr std::string_view inertia = "inertia";
inline constexpr std::array columns{volume, lcg, tcg, vcg, inertia};
}

namespace condition {
inline constexpr std::string_view kind = "kind";
inline constexpr std::string_view id = "id";
inline constexpr std::string_view mass = "mass";
inline constexpr std::string_view fillPercent = "fillPercent";
inline constexpr std::string_view density = "density";
inline constexpr std::string_view phase = "phase";
inline constexpr std::string_view lcg = "lcg";
inline constexpr std::string_view tcg = "tcg";
inline constexpr std::string_view vcg = "vcg";
inline constexpr std::string_view fsm = "fsm";
inline constexpr std::array columns{kind, id, mass, fillPercent, density, phase, lcg, tcg, vcg, fsm};
// The columns that a row of each kind uses; a value in any other column is an error.
inline constexpr std::array tankColumns{kind, id, mass, fillPercent, density, phase};
inline constexpr std::array weightColumns{kind, id, mass, lcg, tcg, vcg, fsm};
inline constexpr std::array nameColumns{kind, id};

inline constexpr std::string_view tank = "tank";
inline constexpr std::string_view weight = "weight";
inline constexpr std::string_view name = "name";
inline constexpr std::string_view liquid = "liquid";
inline constexpr std::string_view solid = "solid";
}

}
