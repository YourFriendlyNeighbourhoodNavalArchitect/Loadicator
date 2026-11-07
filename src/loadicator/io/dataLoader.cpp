#include "loadicator/io/dataLoader.h"

#include "loadicator/core/error.h"
#include "loadicator/io/csv.h"
#include "loadicator/text/numberParsing.h"

#include <algorithm>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace loadicator::io {

namespace fs = std::filesystem;

namespace {

[[nodiscard]] Length lengthAt(const CsvRecord& record, std::string_view column) {
    return Length{record.number(column)};
}

[[nodiscard]] Position positionAt(const CsvRecord& record, std::string_view x, std::string_view y,
                                  std::string_view z) {
    return Position{lengthAt(record, x), lengthAt(record, y), lengthAt(record, z)};
}

[[nodiscard]] CargoPhase phaseAt(const CsvRecord& record) {
    const std::string_view phase = record.text("phase");
    if (phase.empty() || phase == "liquid") {
        return CargoPhase::liquid;
    }
    if (phase == "solid") {
        return CargoPhase::solid;
    }
    throw LoadicatorError(record.location() + ": phase must be 'liquid' or 'solid', got '" +
                          std::string(phase) + "'");
}

[[nodiscard]] TankLoad tankLoadAt(const CsvRecord& record) {
    const auto mass = record.optionalNumber("mass");
    const auto fillPercent = record.optionalNumber("fillPercent");
    if (mass.has_value() == fillPercent.has_value()) {
        throw LoadicatorError(record.location() + ": a tank needs exactly one of 'mass' or 'fillPercent'");
    }
    if (fillPercent && (*fillPercent < 0.0 || *fillPercent > 100.0)) {
        throw LoadicatorError(record.location() + ": fillPercent must be within 0 .. 100");
    }
    if (mass && *mass < 0.0) {
        throw LoadicatorError(record.location() + ": mass may not be negative");
    }
    const double density = record.number("density");
    if (density <= 0.0) {
        throw LoadicatorError(record.location() + ": density must be positive");
    }
    return TankLoad{.compartment = CompartmentId{std::string(record.text("id"))},
                    .amount = mass ? TankAmount{Mass{*mass}} : TankAmount{ratioFromPercent(*fillPercent)},
                    .density = Density{density},
                    .phase = phaseAt(record)};
}

[[nodiscard]] WeightItem weightAt(const CsvRecord& record) {
    if (record.optionalNumber("fillPercent")) {
        throw LoadicatorError(record.location() + ": a weight is given by mass, not fillPercent");
    }
    return WeightItem{.name = std::string(record.text("id")),
                      .mass = Mass{record.number("mass")},
                      .centre = positionAt(record, "lcg", "tcg", "vcg"),
                      .freeSurfaceMoment = Moment{record.optionalNumber("fsm").value_or(0.0)}};
}

} // namespace

ShipParticulars loadShipParticulars(const fs::path& file) {
    const CsvDocument document = CsvDocument::readFile(file);
    document.requireColumns({"parameter", "value"});
    std::map<std::string, double, std::less<>> values;
    for (const CsvRecord& record : document.records()) {
        values.emplace(std::string(record.text("parameter")), record.number("value"));
    }
    const auto lengthOf = [&](std::string_view name) {
        const auto found = values.find(name);
        if (found == values.end()) {
            throw LoadicatorError(document.sourceName() + ": missing parameter '" + std::string(name) + "'");
        }
        return Length{found->second};
    };
    return ShipParticulars{.lengthBetweenPerpendiculars = lengthOf("lengthBetweenPerpendiculars"),
                           .breadthMoulded = lengthOf("breadthMoulded"),
                           .depthMoulded = lengthOf("depthMoulded"),
                           .designDraught = lengthOf("designDraught"),
                           .referencePointX = lengthOf("referencePointX")};
}

HydrostaticTable loadHydrostatics(const fs::path& evenKeelFile, const fs::path& trimGridFile) {
    const CsvDocument evenKeelDocument = CsvDocument::readFile(evenKeelFile);
    evenKeelDocument.requireColumns({"draught", "displacement", "lcf", "lcb", "vcb", "kmt", "mctOneCm"});
    std::vector<EvenKeelHydrostatics> evenKeel;
    for (const CsvRecord& record : evenKeelDocument.records()) {
        evenKeel.push_back(EvenKeelHydrostatics{.draught = lengthAt(record, "draught"),
                                                .displacement = Mass{record.number("displacement")},
                                                .lcf = lengthAt(record, "lcf"),
                                                .lcb = lengthAt(record, "lcb"),
                                                .vcb = lengthAt(record, "vcb"),
                                                .kmt = lengthAt(record, "kmt"),
                                                .mctOneCm = Moment{record.number("mctOneCm")}});
    }

    const CsvDocument gridDocument = CsvDocument::readFile(trimGridFile);
    gridDocument.requireColumns({"draught", "trim", "displacement", "lcb", "kmt"});
    std::vector<TrimGridPoint> grid;
    for (const CsvRecord& record : gridDocument.records()) {
        grid.push_back(
            TrimGridPoint{.draught = lengthAt(record, "draught"),
                          .trim = lengthAt(record, "trim"),
                          .values = TrimmedHydrostatics{.displacement = Mass{record.number("displacement")},
                                                        .lcb = lengthAt(record, "lcb"),
                                                        .kmt = lengthAt(record, "kmt")}});
    }
    return HydrostaticTable(evenKeel, grid);
}

CompartmentTable loadCompartmentTable(const fs::path& file) {
    const CsvDocument document = CsvDocument::readFile(file);
    document.requireColumns({"volume", "lcg", "tcg", "vcg", "inertia"});
    const auto id = document.metadata("id");
    if (!id) {
        throw LoadicatorError(document.sourceName() + ": missing '# id:' metadata");
    }

    std::vector<CompartmentTable::Row> rows;
    for (const CsvRecord& record : document.records()) {
        rows.push_back(
            CompartmentTable::Row{.volume = Volume{record.number("volume")},
                                  .centroid = positionAt(record, "lcg", "tcg", "vcg"),
                                  .freeSurfaceInertia = SecondMomentOfArea{record.number("inertia")}});
    }
    if (rows.empty()) {
        throw LoadicatorError(document.sourceName() + ": no rows");
    }

    Volume capacity = rows.back().volume;
    if (const auto capacityText = document.metadata("capacity")) {
        const auto value = text::parseNumber(*capacityText);
        if (!value) {
            throw LoadicatorError(document.sourceName() + ": capacity metadata is not a number");
        }
        capacity = Volume{*value};
    }
    try {
        return CompartmentTable(CompartmentId{*id}, document.metadata("description").value_or(""), capacity,
                                rows);
    } catch (const LoadicatorError& error) {
        throw LoadicatorError(document.sourceName() + ": " + error.what());
    }
}

CompartmentLibrary loadCompartments(const fs::path& directory) {
    if (!fs::is_directory(directory)) {
        throw LoadicatorError("compartment directory '" + directory.string() + "' does not exist");
    }
    std::vector<fs::path> files;
    for (const auto& entry : fs::directory_iterator(directory)) {
        if (entry.is_regular_file() && entry.path().extension() == ".csv") {
            files.push_back(entry.path());
        }
    }
    std::sort(files.begin(), files.end());

    CompartmentLibrary library;
    for (const fs::path& file : files) {
        library.add(loadCompartmentTable(file));
    }
    return library;
}

Ship loadShip(const fs::path& dataDirectory) {
    return Ship{.particulars = loadShipParticulars(dataDirectory / "ship.csv"),
                .hydrostatics = loadHydrostatics(dataDirectory / "hydrostatics" / "evenKeel.csv",
                                                 dataDirectory / "hydrostatics" / "trimGrid.csv"),
                .compartments = loadCompartments(dataDirectory / "compartments")};
}

LoadingCondition loadLoadingCondition(const fs::path& file) {
    const CsvDocument document = CsvDocument::readFile(file);
    document.requireColumns(
        {"kind", "id", "mass", "fillPercent", "density", "phase", "lcg", "tcg", "vcg", "fsm"});

    LoadingCondition condition{.name = document.metadata("name").value_or(file.stem().string())};
    for (const CsvRecord& record : document.records()) {
        const std::string_view kind = record.text("kind");
        if (kind == "tank") {
            condition.tankLoads.push_back(tankLoadAt(record));
        } else if (kind == "weight") {
            condition.fixedWeights.push_back(weightAt(record));
        } else {
            throw LoadicatorError(record.location() + ": kind must be 'tank' or 'weight', got '" +
                                  std::string(kind) + "'");
        }
    }
    if (condition.tankLoads.empty() && condition.fixedWeights.empty()) {
        throw LoadicatorError(document.sourceName() + ": loading condition is empty");
    }
    return condition;
}

} // namespace loadicator::io
