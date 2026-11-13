#include "io/dataLoader.h"

#include "io/csv.h"
#include "io/schema.h"
#include "util/error.h"

#include <algorithm>
#include <initializer_list>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace loadicator::io {

namespace fs = std::filesystem;

namespace {

[[nodiscard]] Length length(const CsvRow& row, std::string_view column) {
    return Length{row.number(column)};
}

[[nodiscard]] Position position(const CsvRow& row) {
    return Position{length(row, "lcg"), length(row, "tcg"), length(row, "vcg")};
}

// Fails if a row of a loading condition has a value in a column that rows of its kind do not use.
void requireOnly(const CsvRow& row, std::initializer_list<std::string_view> used) {
    for (const std::string_view column : schema::conditionColumns) {
        if (std::ranges::find(used, column) == used.end() && !row.text(column).empty()) {
            throw LoadicatorError(row.location() + ": a row of kind '" + std::string(row.text("kind")) +
                                  "' takes no value in column '" + std::string(column) + "'");
        }
    }
}

[[nodiscard]] TankLoad tankLoad(const CsvRow& row) {
    requireOnly(row, {"kind", "id", "mass", "fillPercent", "density", "phase"});
    const auto mass = row.optionalNumber("mass");
    const auto fillPercent = row.optionalNumber("fillPercent");
    if (mass.has_value() == fillPercent.has_value()) {
        throw LoadicatorError(row.location() + ": a tank needs exactly one of 'mass' or 'fillPercent'");
    }
    const std::string_view phase = row.text("phase");
    if (!phase.empty() && phase != "liquid" && phase != "solid") {
        throw LoadicatorError(row.location() + ": phase must be 'liquid' or 'solid', got '" +
                              std::string(phase) + "'");
    }
    return TankLoad{.compartment = std::string(row.text("id")),
                    .amount = mass ? std::variant<Mass, Ratio>{Mass{*mass}} : Ratio{*fillPercent / 100.0},
                    .density = Density{row.number("density")},
                    .phase = phase == "solid" ? CargoPhase::solid : CargoPhase::liquid};
}

[[nodiscard]] WeightItem weight(const CsvRow& row) {
    requireOnly(row, {"kind", "id", "mass", "lcg", "tcg", "vcg", "fsm"});
    return WeightItem{.name = std::string(row.text("id")),
                      .mass = Mass{row.number("mass")},
                      .centre = position(row),
                      .freeSurfaceMoment = Moment{row.optionalNumber("fsm").value_or(0.0)}};
}

[[nodiscard]] ShipParticulars loadShipParticulars(const fs::path& file) {
    const std::vector<CsvRow> rows = readCsv(file, schema::shipColumns);
    const auto parameter = [&](std::string_view name) {
        const auto row = std::ranges::find(
            rows, name, [](const CsvRow& candidate) { return candidate.text("parameter"); });
        if (row == rows.end()) {
            throw LoadicatorError(file.string() + ": missing parameter '" + std::string(name) + "'");
        }
        return length(*row, "value");
    };
    return ShipParticulars{.lengthBetweenPerpendiculars = parameter("lengthBetweenPerpendiculars"),
                           .referencePointX = parameter("referencePointX")};
}

[[nodiscard]] HydrostaticTable loadHydrostatics(const fs::path& dataDirectory) {
    std::vector<EvenKeelHydrostatics> evenKeel;
    for (const CsvRow& row : readCsv(dataDirectory / schema::evenKeelFile, schema::evenKeelColumns)) {
        evenKeel.push_back(EvenKeelHydrostatics{
            .draught = length(row, "draught"), .lcf = length(row, "lcf"), .vcb = length(row, "vcb")});
    }
    std::vector<TrimGridPoint> grid;
    for (const CsvRow& row : readCsv(dataDirectory / schema::trimGridFile, schema::trimGridColumns)) {
        grid.push_back(TrimGridPoint{.draught = length(row, "draught"),
                                     .trim = length(row, "trim"),
                                     .displacement = Mass{row.number("displacement")},
                                     .lcb = length(row, "lcb"),
                                     .kmt = length(row, "kmt")});
    }
    return HydrostaticTable(evenKeel, std::move(grid));
}

[[nodiscard]] CompartmentLibrary loadCompartments(const fs::path& directory) {
    CompartmentLibrary library;
    for (const auto& entry : fs::directory_iterator(directory)) {
        if (entry.path().extension() != ".csv") {
            continue;
        }
        std::vector<CompartmentTable::Row> rows;
        for (const CsvRow& row : readCsv(entry.path(), schema::compartmentColumns)) {
            rows.push_back(
                CompartmentTable::Row{.volume = Volume{row.number("volume")},
                                      .centroid = position(row),
                                      .freeSurfaceInertia = SecondMomentOfArea{row.number("inertia")}});
        }
        library.emplace(entry.path().stem().string(),
                        withContext(entry.path().string(), [&] { return CompartmentTable(rows); }));
    }
    return library;
}

}

Ship loadShip(const fs::path& dataDirectory) {
    return Ship{.particulars = loadShipParticulars(dataDirectory / schema::shipFile),
                .hydrostatics = loadHydrostatics(dataDirectory),
                .compartments = loadCompartments(dataDirectory / schema::compartmentDirectory)};
}

// The condition is named by its row of kind 'name', or else after its file.
LoadingCondition loadLoadingCondition(const fs::path& file) {
    LoadingCondition condition;
    std::optional<std::string> nameLocation;
    for (const CsvRow& row : readCsv(file, schema::conditionColumns)) {
        const std::string_view kind = row.text("kind");
        if (kind == "tank") {
            condition.tanks.push_back(tankLoad(row));
        } else if (kind == "weight") {
            condition.weights.push_back(weight(row));
        } else if (kind == "name") {
            requireOnly(row, {"kind", "id"});
            if (nameLocation) {
                throw LoadicatorError(row.location() + ": the condition is already named at " +
                                      *nameLocation);
            }
            condition.name = row.text("id");
            nameLocation = row.location();
        } else {
            throw LoadicatorError(row.location() + ": kind must be 'tank', 'weight' or 'name', got '" +
                                  std::string(kind) + "'");
        }
    }
    if (!nameLocation) {
        condition.name = file.stem().string();
    }
    return condition;
}

}
