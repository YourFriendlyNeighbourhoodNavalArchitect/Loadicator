#include "extractor.h"

#include "io/schema.h"
#include "util/error.h"

#include <array>
#include <format>
#include <fstream>
#include <initializer_list>
#include <span>
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
    // No negative zero.
    if (text.starts_with('-') && text.find_first_not_of("-0.") == std::string::npos) {
        text.erase(0, 1);
    }
    return text;
}

template <typename Fields>
void appendLine(std::string& csv, const Fields& fields) {
    std::string_view separator;
    for (const std::string_view field : fields) {
        csv += separator;
        csv += field;
        separator = ",";
    }
    csv += '\n';
}

void appendLine(std::string& csv, std::initializer_list<std::string> fields) {
    appendLine(csv, std::span(fields.begin(), fields.size()));
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

// The rows of each file follow the order of its columns in the schema.

[[nodiscard]] std::string shipFile(const ShipParticulars& ship) {
    std::string csv;
    appendLine(csv, schema::shipColumns);
    appendLine(csv, {"lengthBetweenPerpendiculars", number(ship.lengthBetweenPerpendiculars.value(), 3)});
    appendLine(csv, {"referencePointX", number(ship.referencePointX.value(), 3)});
    return csv;
}

[[nodiscard]] std::string evenKeelFile(const std::vector<EvenKeelHydrostatics>& rows) {
    std::string csv;
    appendLine(csv, schema::evenKeelColumns);
    for (const EvenKeelHydrostatics& row : rows) {
        appendLine(csv,
                   {number(row.draught.value(), 3), number(row.lcf.value(), 3), number(row.vcb.value(), 3)});
    }
    return csv;
}

[[nodiscard]] std::string trimGridFile(const std::vector<TrimGridPoint>& points) {
    std::string csv;
    appendLine(csv, schema::trimGridColumns);
    for (const TrimGridPoint& point : points) {
        appendLine(csv, {number(point.draught.value(), 3), number(point.trim.value(), 3),
                         number(point.displacement.value(), 1), number(point.lcb.value(), 3),
                         number(point.kmt.value(), 3)});
    }
    return csv;
}

[[nodiscard]] std::string compartmentFile(const CompartmentData& table) {
    std::string csv;
    appendLine(csv, schema::compartmentColumns);
    for (const CompartmentTable::Row& row : table.rows) {
        appendLine(csv, {number(row.volume.value(), 2), number(row.centroid.x.value(), 3),
                         number(row.centroid.y.value(), 3), number(row.centroid.z.value(), 3),
                         number(row.freeSurfaceInertia.value(), 1)});
    }
    return csv;
}

[[nodiscard]] std::string conditionFile(const LoadingCondition& condition) {
    std::string csv;
    appendLine(csv, schema::conditionColumns);
    appendLine(csv, {"name", condition.name, "", "", "", "", "", "", "", ""});
    for (const TankLoad& tank : condition.tanks) {
        appendLine(csv, {"tank", tank.compartment, number(std::get<Mass>(tank.amount).value(), 1), "",
                         number(tank.density.value(), 3),
                         tank.phase == CargoPhase::solid ? "solid" : "liquid", "", "", "", ""});
    }
    for (const WeightItem& weight : condition.weights) {
        appendLine(csv, {"weight", weight.name, number(weight.mass.value(), 1), "", "", "",
                         number(weight.centre.x.value(), 2), number(weight.centre.y.value(), 2),
                         number(weight.centre.z.value(), 2), number(weight.freeSurfaceMoment.value(), 2)});
    }
    return csv;
}

[[nodiscard]] std::string bookResultsFile(const std::vector<BookCondition>& conditions) {
    struct Column {
        std::string_view name;
        int decimals;
    };
    constexpr std::array columns{
        Column{"displacement", 1},   Column{"lcg", 2},  Column{"tcg", 2},     Column{"kg", 2},
        Column{"draught", 3},        Column{"trim", 3}, Column{"heel", 1},    Column{"draughtAft", 3},
        Column{"draughtForward", 3}, Column{"kmt", 2},  Column{"gmSolid", 2}, Column{"gm", 2}};

    std::vector<std::string> fields{"condition"};
    for (const Column& column : columns) {
        fields.emplace_back(column.name);
    }
    std::string csv;
    appendLine(csv, fields);
    for (const BookCondition& book : conditions) {
        fields = {schema::bookConditionKey(book.number)};
        for (const Column& column : columns) {
            fields.push_back(number(book.results.at(std::string(column.name)), column.decimals));
        }
        appendLine(csv, fields);
    }
    return csv;
}

}

void writeShipData(const ShipData& data, const fs::path& dataDirectory) {
    const fs::path compartments = dataDirectory / schema::compartmentDirectory;
    const fs::path conditions = dataDirectory / schema::conditionDirectory;
    fs::remove_all(compartments);
    fs::remove_all(conditions);

    writeFile(dataDirectory / schema::shipFile, shipFile(data.hydrostatics.particulars));
    writeFile(dataDirectory / schema::evenKeelFile, evenKeelFile(data.hydrostatics.evenKeel));
    writeFile(dataDirectory / schema::trimGridFile, trimGridFile(data.hydrostatics.trimGrid));
    for (const CompartmentData& table : data.compartments) {
        writeFile(compartments / (table.id + ".csv"), compartmentFile(table));
    }
    for (const BookCondition& book : data.conditions) {
        writeFile(conditions / (schema::bookConditionKey(book.number) + ".csv"),
                  conditionFile(book.condition));
    }
    writeFile(dataDirectory / schema::bookResultsFile, bookResultsFile(data.conditions));
}

}
