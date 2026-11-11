#include "compartmentTables.h"
#include "dataWriter.h"
#include "hydrostaticTables.h"
#include "loadingConditions.h"
#include "sourceText.h"
#include "verification.h"

#include <algorithm>
#include <exception>
#include <filesystem>
#include <iostream>
#include <set>
#include <string>
#include <vector>

namespace {

namespace fs = std::filesystem;
using namespace loadicator::extractor;

[[nodiscard]] ShipData extract(const fs::path& sourceDirectory, const fs::path& pdfTextDirectory) {
    ShipData data;
    data.hydrostatics = extractHydrostatics(SourceText::readFile(pdfTextDirectory / "hydrostaticTables.txt"));

    data.compartments = extractSoundingTables(SourceText::readFile(sourceDirectory / "soundingTables.txt"));
    std::vector<fs::path> holdFiles;
    for (const auto& entry : fs::directory_iterator(sourceDirectory / "cargoHolds")) {
        holdFiles.push_back(entry.path());
    }
    std::sort(holdFiles.begin(), holdFiles.end());
    for (const fs::path& file : holdFiles) {
        data.compartments.push_back(extractHoldTable(SourceText::readFile(file)));
    }

    std::set<std::string, std::less<>> tabulated;
    for (const CompartmentData& table : data.compartments) {
        tabulated.insert(table.id);
    }
    data.conditions = extractLoadingConditions(
        SourceText::readFile(pdfTextDirectory / "trimAndStabilityBook.txt"), tabulated);
    return data;
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
