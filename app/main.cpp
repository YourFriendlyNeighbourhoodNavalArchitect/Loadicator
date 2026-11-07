#include "commandLine.h"
#include "loadicator/calc/analysis.h"
#include "loadicator/core/error.h"
#include "loadicator/io/dataLoader.h"
#include "loadicator/report/reportWriter.h"

#include <exception>
#include <fstream>
#include <iostream>
#include <string_view>
#include <vector>

#ifndef LOADICATOR_DEFAULT_DATA_DIR
#define LOADICATOR_DEFAULT_DATA_DIR "data"
#endif

namespace {

using namespace loadicator;

int run(const app::Options& options) {
    const Ship ship = io::loadShip(options.dataDirectory);
    const LoadingCondition condition = io::loadLoadingCondition(options.condition);
    const Analysis analysis = analyzeLoadingCondition(ship, condition, options.method);

    if (!options.outputFile) {
        writeReport(std::cout, analysis);
        return 0;
    }
    std::ofstream file(*options.outputFile);
    if (!file) {
        throw LoadicatorError("cannot write '" + options.outputFile->string() + "'");
    }
    writeReport(file, analysis);
    std::cout << "Report written to " << options.outputFile->string() << '\n';
    return 0;
}

} // namespace

int main(int argumentCount, char** argumentValues) {
    const std::vector<std::string_view> arguments(argumentValues + 1, argumentValues + argumentCount);
    const auto parsed = app::parseCommandLine(arguments, LOADICATOR_DEFAULT_DATA_DIR);

    if (std::holds_alternative<app::HelpRequested>(parsed)) {
        std::cout << app::usage();
        return 0;
    }
    if (const auto* error = std::get_if<std::string>(&parsed)) {
        std::cerr << "loadicator: " << *error << "\n\n" << app::usage();
        return 2;
    }
    try {
        return run(std::get<app::Options>(parsed));
    } catch (const std::exception& error) {
        std::cerr << "loadicator: " << error.what() << '\n';
        return 1;
    }
}
