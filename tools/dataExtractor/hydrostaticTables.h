#pragma once

#include "sourceText.h"

#include <filesystem>
#include <vector>

namespace loadicator::extractor {

struct ShipParticularsData {
    double lengthBetweenPerpendiculars = 0.0;
    double breadthMoulded = 0.0;
    double depthMoulded = 0.0;
    double designDraught = 0.0;
    double referencePointX = 0.0; ///< RFP, m forward of AP
};

/// Even-keel hydrostatics; longitudinal positions already converted to "m from AP".
struct EvenKeelRow {
    double draught = 0.0;
    double displacement = 0.0;
    double lcf = 0.0;
    double lcb = 0.0;
    double vcb = 0.0;
    double kmt = 0.0;
    double kml = 0.0;
    double mctOneCm = 0.0;
    double tpc = 0.0;
};

/// Free-trim hydrostatics at (draught at RFP, trim by the stern).
struct TrimGridPoint {
    double draught = 0.0;
    double trim = 0.0;
    double displacement = 0.0;
    double lcb = 0.0;
    double kmt = 0.0;
};

struct HydrostaticData {
    ShipParticularsData ship;
    std::vector<EvenKeelRow> evenKeel;
    std::vector<TrimGridPoint> trimGrid;
};

/// Parses the `pdftotext -raw` rendering of the hydrostatic tables.
[[nodiscard]] HydrostaticData extractHydrostatics(const SourceText& source);

/// Writes ship.csv, hydrostatics/evenKeel.csv and hydrostatics/trimGrid.csv.
void writeHydrostatics(const HydrostaticData& data, const std::filesystem::path& outputDirectory);

} // namespace loadicator::extractor
