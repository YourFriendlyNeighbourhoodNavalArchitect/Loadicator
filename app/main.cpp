#include "calc/analysis.h"
#include "commandLine.h"
#include "io/dataLoader.h"
#include "io/reportWriter.h"
#include "util/error.h"

#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string_view>
#include <vector>

namespace {

using namespace loadicator;

[[nodiscard]] std::filesystem::path defaultDataDirectory() {
    const char* const fromEnvironment = std::getenv("LOADICATOR_DATA_DIR");
    return fromEnvironment != nullptr ? fromEnvironment : "data";
}

void run(const app::Options& options) {
    if (!std::filesystem::is_directory(options.dataDirectory)) {
        throw LoadicatorError("no ship data directory '" + options.dataDirectory.string() +
                              "'; name it with --data-dir or in LOADICATOR_DATA_DIR");
    }
    const Ship ship = io::loadShip(options.dataDirectory);
    const LoadingCondition condition = io::loadLoadingCondition(options.condition);
    const Analysis analysis = analyzeLoadingCondition(ship, condition);

    if (!options.outputFile) {
        io::writeReport(std::cout, analysis);
        return;
    }
    std::ofstream file(*options.outputFile);
    io::writeReport(file, analysis);
    file.close();
    if (!file) {
        throw LoadicatorError("cannot write '" + options.outputFile->string() + "'");
    }
    std::cout << "Report written to " << options.outputFile->string() << '\n';
}

}

int main(int argumentCount, char** argumentValues) {
    const std::vector<std::string_view> arguments(argumentValues + 1, argumentValues + argumentCount);
    const auto parsed = app::parseCommandLine(arguments, defaultDataDirectory());

    if (std::holds_alternative<app::HelpRequested>(parsed)) {
        std::cout << app::usage();
        return 0;
    }
    if (const auto* error = std::get_if<std::string>(&parsed)) {
        std::cerr << "loadicator: " << *error << "\n\n" << app::usage();
        return 2;
    }
    try {
        run(std::get<app::Options>(parsed));
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "loadicator: " << error.what() << '\n';
        return 1;
    }
}
