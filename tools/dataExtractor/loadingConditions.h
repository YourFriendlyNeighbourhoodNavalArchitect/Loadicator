#pragma once

#include "sourceText.h"

#include <filesystem>
#include <set>
#include <string>
#include <vector>

namespace loadicator::extractor {

/// A compartment with a capacity table, loaded by mass at a given density.
struct TankEntry {
    std::string id;
    double mass = 0.0;
    double density = 0.0;
    bool solid = false; ///< solid bulk cargo has no free surface
};

/// A weight given directly by mass and centre (lightship, crew & stores, untabulated tanks).
struct WeightEntry {
    std::string id;
    double mass = 0.0;
    double lcg = 0.0;
    double tcg = 0.0;
    double vcg = 0.0;
    double fsm = 0.0;
};

/// The book's own results for the condition ("Total weight" and "FLOATING POSITION").
struct BookResult {
    double displacement = 0.0;
    double lcg = 0.0;
    double tcg = 0.0;
    double kg = 0.0;
    double draught = 0.0;
    double trim = 0.0;
    double heel = 0.0;
    double draughtAft = 0.0;
    double draughtForward = 0.0;
    double kmt = 0.0;
    double gmSolid = 0.0;
    double gmCorrection = 0.0;
    double gm = 0.0;
};

struct LoadingConditionData {
    int number = 0;
    std::string title;
    std::vector<TankEntry> tanks;
    std::vector<WeightEntry> weights;
    BookResult result;
};

/// Parses LOADING CONDITION - COND01 … COND31 from the `pdftotext -raw` rendering of the trim
/// and stability book. Entries whose id is in tabulatedCompartments become tank entries, all
/// others weight entries. Each condition is checked against the book's total weight.
[[nodiscard]] std::vector<LoadingConditionData>
extractLoadingConditions(const SourceText& source,
                         const std::set<std::string, std::less<>>& tabulatedCompartments);

/// Writes conditions/condNN.csv and conditions/expected.csv.
void writeLoadingConditions(const std::vector<LoadingConditionData>& conditions,
                            const std::filesystem::path& outputDirectory);

} // namespace loadicator::extractor
