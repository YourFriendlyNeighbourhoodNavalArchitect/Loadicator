#include "dataWriter.h"

#include "io/schema.h"
#include "util/error.h"

#include <array>
#include <format>
#include <fstream>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace loadicator::extractor {

namespace {

namespace fs = std::filesystem;
namespace schema = io::schema;

[[nodiscard]] std::string number(double value, int decimals) {
    std::string text = std::format("{:.{}f}", value, decimals);
    if (text.starts_with('-') && text.find_first_not_of("-0.") == std::string::npos) {
        text.erase(0, 1);
    }
    return text;
}

class CsvText {
public:
    void header(const std::vector<std::string_view>& columns) {
        columnCount = columns.size();
        line(columns);
    }

    void row(const std::vector<std::string>& fields) { line(fields); }

    [[nodiscard]] const std::string& str() const { return text; }

private:
    template <typename Fields>
    void line(const Fields& fields) {
        if (fields.size() != columnCount) {
            throw LoadicatorError(
                std::format("a CSV row has {} fields under a header of {}", fields.size(), columnCount));
        }
        std::string_view separator;
        for (const std::string_view field : fields) {
            if (field.find_first_of(",\n") != std::string_view::npos) {
                throw LoadicatorError("the CSV field '" + std::string(field) +
                                      "' holds a comma or a line break");
            }
            text += separator;
            text += field;
            separator = ",";
        }
        text += '\n';
    }

    std::string text;
    std::size_t columnCount = 0;
};

[[nodiscard]] std::string shipFile(const ShipParticulars& ship) {
    CsvText csv;
    csv.header({schema::ship::parameter, schema::ship::value});
    csv.row({std::string(schema::ship::lengthBetweenPerpendiculars),
             number(ship.lengthBetweenPerpendiculars.value(), 3)});
    csv.row({std::string(schema::ship::referencePointX), number(ship.referencePointX.value(), 3)});
    return csv.str();
}

[[nodiscard]] std::string evenKeelFile(const std::vector<EvenKeelHydrostatics>& rows) {
    CsvText csv;
    csv.header({schema::evenKeel::draught, schema::evenKeel::lcf, schema::evenKeel::vcb});
    for (const EvenKeelHydrostatics& row : rows) {
        csv.row({number(row.draught.value(), 3), number(row.lcf.value(), 3), number(row.vcb.value(), 3)});
    }
    return csv.str();
}

[[nodiscard]] std::string trimGridFile(const std::vector<TrimGridPoint>& points) {
    CsvText csv;
    csv.header({schema::trimGrid::draught, schema::trimGrid::trim, schema::trimGrid::displacement,
                schema::trimGrid::lcb, schema::trimGrid::kmt});
    for (const TrimGridPoint& point : points) {
        csv.row({number(point.draught.value(), 3), number(point.trim.value(), 3),
                 number(point.displacement.value(), 1), number(point.lcb.value(), 3),
                 number(point.kmt.value(), 3)});
    }
    return csv.str();
}

[[nodiscard]] std::string compartmentFile(const CompartmentData& table) {
    CsvText csv;
    csv.header({schema::compartment::volume, schema::compartment::lcg, schema::compartment::tcg,
                schema::compartment::vcg, schema::compartment::inertia});
    for (const CompartmentTable::Row& row : table.rows) {
        csv.row({number(row.volume.value(), 2), number(row.centroid.x.value(), 3),
                 number(row.centroid.y.value(), 3), number(row.centroid.z.value(), 3),
                 number(row.freeSurfaceInertia.value(), 1)});
    }
    return csv.str();
}

[[nodiscard]] std::string conditionFile(const LoadingCondition& condition) {
    namespace columns = schema::condition;
    CsvText csv;
    csv.header({columns::kind, columns::id, columns::mass, columns::fillPercent, columns::density,
                columns::phase, columns::lcg, columns::tcg, columns::vcg, columns::fsm});
    csv.row({std::string(columns::name), condition.name, "", "", "", "", "", "", "", ""});
    for (const TankLoad& tank : condition.tankLoads) {
        csv.row({std::string(columns::tank), tank.compartment.value(),
                 number(std::get<Mass>(tank.amount).value(), 1), "", number(tank.density.value(), 3),
                 std::string(tank.phase == CargoPhase::solid ? columns::solid : columns::liquid), "", "", "",
                 ""});
    }
    for (const WeightItem& weight : condition.fixedWeights) {
        csv.row({std::string(columns::weight), weight.name, number(weight.mass.value(), 1), "", "", "",
                 number(weight.centre.x.value(), 2), number(weight.centre.y.value(), 2),
                 number(weight.centre.z.value(), 2), number(weight.freeSurfaceMoment.value(), 2)});
    }
    return csv.str();
}

[[nodiscard]] std::string bookResultsFile(const std::vector<BookCondition>& conditions) {
    namespace columns = schema::bookResults;
    struct Column {
        std::string_view name;
        int decimals;
    };
    constexpr std::array resultColumns{
        Column{columns::displacement, 1}, Column{columns::lcg, 2},        Column{columns::tcg, 2},
        Column{columns::kg, 2},           Column{columns::draught, 3},    Column{columns::trim, 3},
        Column{columns::heel, 1},         Column{columns::draughtAft, 3}, Column{columns::draughtForward, 3},
        Column{columns::kmt, 2},          Column{columns::gmSolid, 2},    Column{columns::gm, 2}};

    CsvText csv;
    std::vector<std::string_view> header{columns::condition};
    for (const Column& column : resultColumns) {
        header.push_back(column.name);
    }
    csv.header(header);
    for (const BookCondition& book : conditions) {
        std::vector<std::string> fields{schema::bookConditionKey(book.number)};
        for (const Column& column : resultColumns) {
            fields.push_back(number(book.results.at(column.name), column.decimals));
        }
        csv.row(fields);
    }
    return csv.str();
}

void writeFile(const fs::path& path, const std::string& text) {
    fs::create_directories(path.parent_path());
    std::ofstream file(path, std::ios::binary);
    file << text;
    file.close();
    if (!file) {
        throw LoadicatorError("cannot write '" + path.string() + "'");
    }
}

void removeCsvFiles(const fs::path& directory) {
    if (!fs::is_directory(directory)) {
        return;
    }
    std::vector<fs::path> files;
    for (const auto& entry : fs::directory_iterator(directory)) {
        if (entry.path().extension() == schema::files::extension) {
            files.push_back(entry.path());
        }
    }
    for (const fs::path& file : files) {
        fs::remove(file);
    }
}

}

void writeShipData(const ShipData& data, const fs::path& dataDirectory) {
    const fs::path compartments = dataDirectory / schema::files::compartments;
    const fs::path conditions = dataDirectory / schema::files::conditions;
    removeCsvFiles(compartments);
    removeCsvFiles(conditions);

    writeFile(dataDirectory / schema::files::ship, shipFile(data.hydrostatics.particulars));
    writeFile(dataDirectory / schema::files::evenKeel, evenKeelFile(data.hydrostatics.evenKeel));
    writeFile(dataDirectory / schema::files::trimGrid, trimGridFile(data.hydrostatics.trimGrid));
    for (const CompartmentData& table : data.compartments) {
        writeFile(compartments / (table.id + std::string(schema::files::extension)), compartmentFile(table));
    }
    for (const BookCondition& book : data.conditions) {
        writeFile(conditions /
                      (schema::bookConditionKey(book.number) + std::string(schema::files::extension)),
                  conditionFile(book.condition));
    }
    writeFile(dataDirectory / schema::files::bookResults, bookResultsFile(data.conditions));
}

}
