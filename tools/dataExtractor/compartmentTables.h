#pragma once

#include "sourceText.h"

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace loadicator::extractor {

/// One row of a capacity table, in the units of the data files (m, m³, m⁴).
struct CapacityRow {
    double sounding = 0.0;
    double volume = 0.0;
    double lcg = 0.0;
    double tcg = 0.0;
    double vcg = 0.0;
    double inertia = 0.0;
};

struct CompartmentTableData {
    std::string id;
    std::string description;
    std::string contents;
    std::string provenance;
    double capacity = 0.0; ///< m³ at 100 % fill (may exceed the last tabulated volume)
    std::vector<CapacityRow> rows;
};

/// Parses every compartment of the sounding-tables text (trim 0 volumes).
///
/// The source is a damaged PDF-to-text conversion: in some compartments a run of rows keeps
/// only sounding + 7 volumes, and the missing FILL/LCG/TCG/VCG/IMOM tails follow later on one
/// line with each IMOM glued to the next FILL ("35.798.6" = 35.7 | 98.6). IMOM always has one
/// decimal, so the split is unambiguous; every row is then validated against FILL = V / Vmax.
[[nodiscard]] std::vector<CompartmentTableData> extractSoundingTables(const SourceText& source);

/// Parses one cargo-hold file ("No N Hold", sounding-volume-lcg-tcg-vcg-fsm).
[[nodiscard]] CompartmentTableData extractHoldTable(const SourceText& source, int holdNumber);

/// Writes data/compartments/<id>.csv. Rows whose volume does not increase (the saturated
/// top of a table) are dropped so volume can serve as the interpolation key.
void writeCompartmentTable(const CompartmentTableData& table, const std::filesystem::path& directory);

} // namespace loadicator::extractor
