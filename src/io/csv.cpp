#include "io/csv.h"

#include "io/textFile.h"
#include "util/error.h"
#include "util/text.h"

#include <algorithm>
#include <utility>

namespace loadicator::io {

std::string_view CsvRow::text(const CsvColumn& column) const {
    return column.index ? std::string_view(document->lines[index].fields[*column.index]) : std::string_view();
}

double CsvRow::number(const CsvColumn& column) const {
    const auto value = optionalNumber(column);
    if (!value) {
        throw LoadicatorError(location() + ": column '" + std::string(column.name()) + "' is required");
    }
    return *value;
}

std::optional<double> CsvRow::optionalNumber(const CsvColumn& column) const {
    const std::string_view field = text(column);
    if (field.empty()) {
        return std::nullopt;
    }
    const auto value = text::parseNumber(field);
    if (!value) {
        throw LoadicatorError(location() + ": column '" + std::string(column.name()) +
                              "' is not a number: '" + std::string(field) + "'");
    }
    return value;
}

std::string CsvRow::location() const {
    return document->source + ":" + std::to_string(document->lines[index].number);
}

CsvDocument CsvDocument::readFile(const std::filesystem::path& path) {
    return CsvDocument(readTextFile(path), path.string());
}

CsvDocument::CsvDocument(std::string_view content, std::string sourceName) : source(std::move(sourceName)) {
    std::size_t lineNumber = 0;
    for (const std::string_view rawLine : text::split(content, '\n')) {
        ++lineNumber;
        const std::string_view line = text::trim(rawLine);
        if (line.empty()) {
            continue;
        }

        std::vector<std::string> fields;
        for (const std::string_view field : text::split(line, ',')) {
            fields.emplace_back(text::trim(field));
        }
        if (header.empty()) {
            header = std::move(fields);
        } else if (fields.size() == header.size()) {
            lines.push_back(Line{lineNumber, std::move(fields)});
        } else {
            throw LoadicatorError(source + ":" + std::to_string(lineNumber) + ": expected " +
                                  std::to_string(header.size()) + " fields, found " +
                                  std::to_string(fields.size()));
        }
    }
    if (header.empty()) {
        throw LoadicatorError(source + ": no header row");
    }
}

CsvColumn CsvDocument::column(std::string_view name) const {
    const CsvColumn found = optionalColumn(name);
    if (!found.index) {
        throw LoadicatorError(source + ": missing column '" + std::string(name) + "'");
    }
    return found;
}

CsvColumn CsvDocument::optionalColumn(std::string_view name) const {
    CsvColumn result;
    result.columnName = name;
    if (const auto found = std::find(header.begin(), header.end(), name); found != header.end()) {
        result.index = static_cast<std::size_t>(found - header.begin());
    }
    return result;
}

}
