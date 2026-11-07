#include "../testing.h"
#include "commandLine.h"
#include "loadicator/calc/analysis.h"
#include "loadicator/io/dataLoader.h"
#include "loadicator/report/reportWriter.h"

#include <filesystem>
#include <sstream>
#include <string_view>
#include <vector>

using namespace loadicator;

namespace {

app::ParseResult parse(std::vector<std::string_view> arguments) {
    return app::parseCommandLine(arguments, "dataDir");
}

} // namespace

TEST_CASE("app/commandLine") {
    const auto byNumber = parse({"3"});
    CHECK(std::get<app::Options>(byNumber).condition ==
          std::filesystem::path("dataDir/conditions/cond03.csv"));
    CHECK(std::get<app::Options>(byNumber).method == EquilibriumMethod::freeTrim);

    const auto byPath = parse({"--method", "firstOrder", "--output", "out.txt", "my.csv"});
    const auto& options = std::get<app::Options>(byPath);
    CHECK(options.condition == std::filesystem::path("my.csv"));
    CHECK(options.method == EquilibriumMethod::firstOrder);
    CHECK(options.outputFile == std::filesystem::path("out.txt"));

    CHECK(std::holds_alternative<app::HelpRequested>(parse({"--help"})));
    CHECK(std::holds_alternative<std::string>(parse({})));
    CHECK(std::holds_alternative<std::string>(parse({"--bogus", "3"})));
    CHECK(std::holds_alternative<std::string>(parse({"--method", "magic", "3"})));
    CHECK(std::holds_alternative<std::string>(parse({"3", "4"})));
    CHECK(std::holds_alternative<std::string>(parse({"3", "--output"})));
}

TEST_CASE("app/shipDataLoads") {
    const Ship ship = io::loadShip(LOADICATOR_TEST_DATA_DIR);
    CHECK(ship.compartments.size() == 85); // 76 tanks + 9 holds
    CHECK_NEAR(ship.particulars.lengthBetweenPerpendiculars.value(), 278.2, 1e-9);
    CHECK_NEAR(ship.particulars.referencePointX.value(), 139.1, 1e-9);
    // Audit C3/C4: tanks the original parser lost or merged are complete and monotonic.
    CHECK_NEAR(ship.compartments.at(CompartmentId{"R2.03P"}).capacity().value(), 1664.94, 0.01);
    CHECK_NEAR(ship.compartments.at(CompartmentId{"R2.22P"}).stateAt(Volume{500.0}).centroid.x.value(), 228.3,
               0.3);
}

TEST_CASE("app/reportContainsResults") {
    const Ship ship = io::loadShip(LOADICATOR_TEST_DATA_DIR);
    const auto condition = io::loadLoadingCondition(std::filesystem::path(LOADICATOR_TEST_DATA_DIR) /
                                                    "conditions" / "cond03.csv");
    std::ostringstream report;
    writeReport(report, analyzeLoadingCondition(ship, condition));
    const std::string text = report.str();
    CHECK(text.find("COND03 NORMAL BALLAST-50% BUNKERS") != std::string::npos);
    CHECK(text.find("Draught aft (TA)") != std::string::npos);
    CHECK(text.find("GM fluid") != std::string::npos);
    CHECK(text.find("Lightweight") != std::string::npos);
}
