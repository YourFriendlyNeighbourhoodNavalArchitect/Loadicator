#include "report.h"

#include "calc/analysis.h"
#include "io/dataLoader.h"
#include "io/schema.h"

#include <charconv>
#include <exception>
#include <filesystem>
#include <iostream>
#include <string_view>
#include <vector>

namespace {

using namespace loadicator;
namespace fs = std::filesystem;

constexpr std::string_view usage =
    "Usage: loadicator [--data-dir <dir>] <condition>\n"
    "\n"
    "  <condition>         loading-condition CSV file, or the number of a condition of the book\n"
    "  --data-dir <dir>    ship data directory (default: data)\n";

// A number names the loading condition of the book by that number, if there is one.
[[nodiscard]] fs::path conditionFile(std::string_view argument, const fs::path& dataDirectory) {
    int number = 0;
    const char* const end = argument.data() + argument.size();
    if (const auto [stop, error] = std::from_chars(argument.data(), end, number);
        error == std::errc{} && stop == end) {
        const fs::path bookCondition =
            dataDirectory / io::schema::conditionDirectory / (io::schema::bookConditionKey(number) + ".csv");
        if (fs::exists(bookCondition)) {
            return bookCondition;
        }
    }
    return argument;
}

}

int main(int argumentCount, char** argumentValues) {
    std::vector<std::string_view> arguments(argumentValues + 1, argumentValues + argumentCount);
    fs::path dataDirectory = "data";
    if (arguments.size() == 3 && arguments[0] == "--data-dir") {
        dataDirectory = arguments[1];
        arguments.erase(arguments.begin(), arguments.begin() + 2);
    }
    if (arguments.size() != 1 || arguments[0].starts_with('-')) {
        std::cerr << usage;
        return 2;
    }
    try {
        const Ship ship = io::loadShip(dataDirectory);
        const LoadingCondition condition =
            io::loadLoadingCondition(conditionFile(arguments[0], dataDirectory));
        writeReport(std::cout, analyzeLoadingCondition(ship, condition));
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "loadicator: " << error.what() << '\n';
        return 1;
    }
}
