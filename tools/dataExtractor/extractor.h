#pragma once

#include "sourceText.h"

#include "domain/compartmentTable.h"
#include "domain/hydrostaticTable.h"
#include "domain/loadingCondition.h"
#include "domain/ship.h"

#include <filesystem>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace loadicator::extractor {

struct CompartmentData {
    std::string id;
    std::vector<CompartmentTable::Row> rows;
};

struct HydrostaticData {
    ShipParticulars particulars;
    std::vector<EvenKeelHydrostatics> evenKeel;
    std::vector<TrimGridPoint> trimGrid;
};

// A loading condition of the book and the results printed for it, by column of the results file.
struct BookCondition {
    int number = 0;
    LoadingCondition condition;
    std::map<std::string, double, std::less<>> results;
};

struct ShipData {
    HydrostaticData hydrostatics;
    std::vector<CompartmentData> compartments;
    std::vector<BookCondition> conditions;
};

[[nodiscard]] HydrostaticData extractHydrostatics(const SourceText& source);

[[nodiscard]] std::vector<CompartmentData> extractSoundingTables(const SourceText& source);

[[nodiscard]] CompartmentData extractHoldTable(const SourceText& source);

[[nodiscard]] std::vector<BookCondition> extractLoadingConditions(
    const SourceText& source, const std::set<std::string, std::less<>>& tabulatedCompartments);

void writeShipData(const ShipData& data, const std::filesystem::path& dataDirectory);

}
