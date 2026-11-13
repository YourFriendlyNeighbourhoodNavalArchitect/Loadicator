#include "extractor.h"

#include "calc/analysis.h"
#include "util/error.h"

#include <algorithm>
#include <cmath>
#include <exception>
#include <filesystem>
#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace {

namespace fs = std::filesystem;
using namespace loadicator;
using namespace loadicator::extractor;

// The book prints masses to 0.1 t.
constexpr double massRounding = 0.05;

[[nodiscard]] ShipData extract(const fs::path& sourceDirectory, const fs::path& pdfTextDirectory) {
    HydrostaticData hydrostatics =
        extractHydrostatics(SourceText::readFile(pdfTextDirectory / "hydrostaticTables.txt"));

    std::vector<CompartmentData> compartments =
        extractSoundingTables(SourceText::readFile(sourceDirectory / "soundingTables.txt"));
    std::vector<fs::path> holdFiles;
    for (const auto& entry : fs::directory_iterator(sourceDirectory / "cargoHolds")) {
        holdFiles.push_back(entry.path());
    }
    std::sort(holdFiles.begin(), holdFiles.end());
    for (const fs::path& file : holdFiles) {
        compartments.push_back(extractHoldTable(SourceText::readFile(file)));
    }

    std::set<std::string, std::less<>> tabulated;
    for (const CompartmentData& table : compartments) {
        tabulated.insert(table.id);
    }
    std::vector<BookCondition> conditions = extractLoadingConditions(
        SourceText::readFile(pdfTextDirectory / "trimAndStabilityBook.txt"), tabulated);
    return ShipData{.hydrostatics = std::move(hydrostatics),
                    .compartments = std::move(compartments),
                    .conditions = std::move(conditions)};
}

// Loads the tables with the classes of the program, and checks that each loading condition resolves against
// the capacity tables to the total weight printed in the book.
void verify(const ShipData& data) {
    (void)HydrostaticTable(data.hydrostatics.evenKeel, data.hydrostatics.trimGrid);

    CompartmentLibrary compartments;
    for (const CompartmentData& table : data.compartments) {
        const bool added = withContext(
            table.id, [&] { return compartments.emplace(table.id, CompartmentTable(table.rows)).second; });
        if (!added) {
            throw LoadicatorError("duplicate capacity table " + table.id);
        }
    }

    for (const BookCondition& book : data.conditions) {
        withContext(book.condition.name, [&] {
            const std::vector<WeightItem> weights = resolveLoadingCondition(book.condition, compartments);
            const double total = summarizeWeights(weights).displacement.value();
            const double bookTotal = book.results.at("displacement");
            if (std::abs(total - bookTotal) > massRounding * static_cast<double>(weights.size() + 1)) {
                throw LoadicatorError("the entries sum to " + std::to_string(total) +
                                      " t, but the book gives a total weight of " +
                                      std::to_string(bookTotal) + " t");
            }
        });
    }
}

}

int main(int argumentCount, char** arguments) {
    if (argumentCount != 4) {
        std::cerr << "usage: dataExtractor <sourceDirectory> <pdfTextDirectory> <dataDirectory>\n";
        return 2;
    }
    try {
        const ShipData data = extract(arguments[1], arguments[2]);
        verify(data);
        writeShipData(data, arguments[3]);
        std::cout << "hydrostatics: " << data.hydrostatics.evenKeel.size() << " draughts, "
                  << data.hydrostatics.trimGrid.size() << " points over draught and trim\n"
                  << "capacity tables: " << data.compartments.size() << '\n'
                  << "loading conditions: " << data.conditions.size() << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "dataExtractor: " << error.what() << '\n';
        return 1;
    }
}
