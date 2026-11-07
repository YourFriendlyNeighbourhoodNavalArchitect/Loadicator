// Converts the original ship documents into the CSV files under data/.
//
// Usage: dataExtractor <sourceDirectory> <pdfTextDirectory> <outputDirectory>
//
//   sourceDirectory   data/source (soundingTables.txt, cargoHolds/hold1..9.txt)
//   pdfTextDirectory  `pdftotext -raw` output of the two PDFs:
//                     trimAndStabilityBook.txt, hydrostaticTables.txt
//   outputDirectory   data
//
// tools/extractData.sh runs pdftotext and this program in one step.

#include "compartmentTables.h"
#include "hydrostaticTables.h"
#include "loadicator/core/error.h"
#include "loadingConditions.h"
#include "sourceText.h"

#include <exception>
#include <filesystem>
#include <iostream>
#include <set>
#include <string>

namespace {

namespace fs = std::filesystem;
using namespace loadicator::extractor;

constexpr int holdCount = 9;

void removeCsvFiles(const fs::path& directory) {
    if (!fs::exists(directory)) {
        return;
    }
    for (const auto& entry : fs::directory_iterator(directory)) {
        if (entry.path().extension() == ".csv") {
            fs::remove(entry.path());
        }
    }
}

int run(const fs::path& sourceDirectory, const fs::path& pdfTextDirectory, const fs::path& outputDirectory) {
    const fs::path compartmentDirectory = outputDirectory / "compartments";
    removeCsvFiles(compartmentDirectory);
    removeCsvFiles(outputDirectory / "conditions");
    fs::create_directories(compartmentDirectory);

    std::set<std::string, std::less<>> tabulated;
    for (const auto& table :
         extractSoundingTables(SourceText::readFile(sourceDirectory / "soundingTables.txt"))) {
        writeCompartmentTable(table, compartmentDirectory);
        tabulated.insert(table.id);
    }
    for (int hold = 1; hold <= holdCount; ++hold) {
        const auto source =
            SourceText::readFile(sourceDirectory / "cargoHolds" / ("hold" + std::to_string(hold) + ".txt"));
        const auto table = extractHoldTable(source, hold);
        writeCompartmentTable(table, compartmentDirectory);
        tabulated.insert(table.id);
    }
    std::cout << "compartment tables: " << tabulated.size() << '\n';

    const auto hydrostatics =
        extractHydrostatics(SourceText::readFile(pdfTextDirectory / "hydrostaticTables.txt"));
    writeHydrostatics(hydrostatics, outputDirectory);
    std::cout << "hydrostatics: " << hydrostatics.evenKeel.size() << " even-keel rows, "
              << hydrostatics.trimGrid.size() << " trim-grid points\n";

    const auto conditions = extractLoadingConditions(
        SourceText::readFile(pdfTextDirectory / "trimAndStabilityBook.txt"), tabulated);
    writeLoadingConditions(conditions, outputDirectory);
    std::cout << "loading conditions: " << conditions.size() << '\n';
    return 0;
}

} // namespace

int main(int argumentCount, char** arguments) {
    if (argumentCount != 4) {
        std::cerr << "usage: dataExtractor <sourceDirectory> <pdfTextDirectory> <outputDirectory>\n";
        return 2;
    }
    try {
        return run(arguments[1], arguments[2], arguments[3]);
    } catch (const std::exception& error) {
        std::cerr << "dataExtractor: " << error.what() << '\n';
        return 1;
    }
}
