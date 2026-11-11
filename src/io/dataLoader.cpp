#include "io/dataLoader.h"

#include "io/csv.h"
#include "io/schema.h"
#include "util/error.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

namespace loadicator::io {

namespace fs = std::filesystem;

namespace {

[[nodiscard]] CargoPhase phaseOf(const CsvRow& row, const CsvColumn& column) {
    const std::string_view phase = row.text(column);
    if (phase.empty() || phase == schema::condition::liquid) {
        return CargoPhase::liquid;
    }
    if (phase == schema::condition::solid) {
        return CargoPhase::solid;
    }
    throw LoadicatorError(row.location() + ": phase must be 'liquid' or 'solid', got '" + std::string(phase) +
                          "'");
}

struct ConditionColumns {
    explicit ConditionColumns(const CsvDocument& document)
        : kind(document.column(schema::condition::kind)), id(document.column(schema::condition::id)),
          mass(document.optionalColumn(schema::condition::mass)),
          fillPercent(document.optionalColumn(schema::condition::fillPercent)),
          density(document.optionalColumn(schema::condition::density)),
          phase(document.optionalColumn(schema::condition::phase)),
          lcg(document.optionalColumn(schema::condition::lcg)),
          tcg(document.optionalColumn(schema::condition::tcg)),
          vcg(document.optionalColumn(schema::condition::vcg)),
          fsm(document.optionalColumn(schema::condition::fsm)) {}

    CsvColumn kind, id, mass, fillPercent, density, phase, lcg, tcg, vcg, fsm;
};

[[nodiscard]] TankLoad tankLoadOf(const CsvRow& row, const ConditionColumns& columns) {
    const auto mass = row.optionalNumber(columns.mass);
    const auto fillPercent = row.optionalNumber(columns.fillPercent);
    if (mass.has_value() == fillPercent.has_value()) {
        throw LoadicatorError(row.location() + ": a tank needs exactly one of 'mass' or 'fillPercent'");
    }
    return withContext(row.location(), [&] {
        return TankLoad(CompartmentId{std::string(row.text(columns.id))},
                        mass ? TankAmount{Mass{*mass}} : TankAmount{ratioFromPercent(*fillPercent)},
                        Density{row.number(columns.density)}, phaseOf(row, columns.phase));
    });
}

[[nodiscard]] WeightItem weightOf(const CsvRow& row, const ConditionColumns& columns) {
    if (row.optionalNumber(columns.fillPercent)) {
        throw LoadicatorError(row.location() + ": a weight is given by mass, not fillPercent");
    }
    return WeightItem{.name = std::string(row.text(columns.id)),
                      .mass = Mass{row.number(columns.mass)},
                      .centre = Position{Length{row.number(columns.lcg)}, Length{row.number(columns.tcg)},
                                         Length{row.number(columns.vcg)}},
                      .freeSurfaceMoment = Moment{row.optionalNumber(columns.fsm).value_or(0.0)}};
}

[[nodiscard]] ShipParticulars loadShipParticulars(const fs::path& file) {
    const CsvDocument document = CsvDocument::readFile(file);
    const CsvColumn parameter = document.column(schema::ship::parameter);
    const CsvColumn value = document.column(schema::ship::value);
    const auto lengthOf = [&](std::string_view name) {
        for (const CsvRow row : document.rows()) {
            if (row.text(parameter) == name) {
                return Length{row.number(value)};
            }
        }
        throw LoadicatorError(document.sourceName() + ": missing parameter '" + std::string(name) + "'");
    };
    return ShipParticulars{.lengthBetweenPerpendiculars = lengthOf(schema::ship::lengthBetweenPerpendiculars),
                           .referencePointX = lengthOf(schema::ship::referencePointX)};
}

[[nodiscard]] HydrostaticTable loadHydrostatics(const fs::path& evenKeelFile, const fs::path& trimGridFile) {
    std::vector<EvenKeelHydrostatics> evenKeel;
    {
        const CsvDocument document = CsvDocument::readFile(evenKeelFile);
        const CsvColumn draught = document.column(schema::evenKeel::draught);
        const CsvColumn lcf = document.column(schema::evenKeel::lcf);
        const CsvColumn vcb = document.column(schema::evenKeel::vcb);
        for (const CsvRow row : document.rows()) {
            evenKeel.push_back(EvenKeelHydrostatics{.draught = Length{row.number(draught)},
                                                    .lcf = Length{row.number(lcf)},
                                                    .vcb = Length{row.number(vcb)}});
        }
    }
    std::vector<TrimGridPoint> grid;
    {
        const CsvDocument document = CsvDocument::readFile(trimGridFile);
        const CsvColumn draught = document.column(schema::trimGrid::draught);
        const CsvColumn trim = document.column(schema::trimGrid::trim);
        const CsvColumn displacement = document.column(schema::trimGrid::displacement);
        const CsvColumn lcb = document.column(schema::trimGrid::lcb);
        const CsvColumn kmt = document.column(schema::trimGrid::kmt);
        for (const CsvRow row : document.rows()) {
            grid.push_back(TrimGridPoint{.draught = Length{row.number(draught)},
                                         .trim = Length{row.number(trim)},
                                         .displacement = Mass{row.number(displacement)},
                                         .lcb = Length{row.number(lcb)},
                                         .kmt = Length{row.number(kmt)}});
        }
    }
    return HydrostaticTable(evenKeel, grid);
}

[[nodiscard]] CompartmentTable loadCompartmentTable(const fs::path& file) {
    const CsvDocument document = CsvDocument::readFile(file);
    const CsvColumn volume = document.column(schema::compartment::volume);
    const CsvColumn lcg = document.column(schema::compartment::lcg);
    const CsvColumn tcg = document.column(schema::compartment::tcg);
    const CsvColumn vcg = document.column(schema::compartment::vcg);
    const CsvColumn inertia = document.column(schema::compartment::inertia);

    std::vector<CompartmentTable::Row> rows;
    for (const CsvRow row : document.rows()) {
        rows.push_back(CompartmentTable::Row{
            .volume = Volume{row.number(volume)},
            .centroid = Position{Length{row.number(lcg)}, Length{row.number(tcg)}, Length{row.number(vcg)}},
            .freeSurfaceInertia = SecondMomentOfArea{row.number(inertia)}});
    }
    return withContext(document.sourceName(),
                       [&] { return CompartmentTable(CompartmentId{file.stem().string()}, rows); });
}

[[nodiscard]] CompartmentLibrary loadCompartments(const fs::path& directory) {
    if (!fs::is_directory(directory)) {
        throw LoadicatorError("compartment directory '" + directory.string() + "' does not exist");
    }
    std::vector<fs::path> files;
    for (const auto& entry : fs::directory_iterator(directory)) {
        if (entry.is_regular_file() && entry.path().extension() == schema::files::extension) {
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

}

Ship loadShip(const fs::path& dataDirectory) {
    return Ship{.particulars = loadShipParticulars(dataDirectory / schema::files::ship),
                .hydrostatics = loadHydrostatics(dataDirectory / schema::files::evenKeel,
                                                 dataDirectory / schema::files::trimGrid),
                .compartments = loadCompartments(dataDirectory / schema::files::compartments)};
}

LoadingCondition loadLoadingCondition(const fs::path& file) {
    const CsvDocument document = CsvDocument::readFile(file);
    const ConditionColumns columns(document);

    LoadingCondition condition{.name = file.stem().string()};
    for (const CsvRow row : document.rows()) {
        const std::string_view kind = row.text(columns.kind);
        if (kind == schema::condition::tank) {
            condition.tankLoads.push_back(tankLoadOf(row, columns));
        } else if (kind == schema::condition::weight) {
            condition.fixedWeights.push_back(weightOf(row, columns));
        } else if (kind == schema::condition::name) {
            condition.name = row.text(columns.id);
        } else {
            throw LoadicatorError(row.location() + ": kind must be 'tank', 'weight' or 'name', got '" +
                                  std::string(kind) + "'");
        }
    }
    if (condition.tankLoads.empty() && condition.fixedWeights.empty()) {
        throw LoadicatorError(document.sourceName() + ": loading condition is empty");
    }
    return condition;
}

}
