#include "loadicator/io/csv.h"

#include "loadicator/io/textFile.h"
#include "loadicator/util/error.h"
#include "loadicator/util/text.h"

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

CsvDocument CsvDocument::readFile(const std::filesystem::path& path,
                                  std::span<const std::string_view> knownColumns) {
    return CsvDocument(readTextFile(path), path.string(), knownColumns);
}

CsvDocument::CsvDocument(std::string_view content, std::string sourceName,
                         std::span<const std::string_view> knownColumns)
    : source(std::move(sourceName)) {
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
            checkHeader(knownColumns, lineNumber);
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

void CsvDocument::checkHeader(std::span<const std::string_view> knownColumns, std::size_t lineNumber) const {
    const std::string location = source + ":" + std::to_string(lineNumber);
    for (auto name = header.begin(); name != header.end(); ++name) {
        if (name->empty()) {
            throw LoadicatorError(location + ": column " + std::to_string(name - header.begin() + 1) +
                                  " of the header has no name");
        }
        if (std::find(knownColumns.begin(), knownColumns.end(), *name) == knownColumns.end()) {
            std::string known;
            for (const std::string_view column : knownColumns) {
                known += (known.empty() ? "" : ", ") + std::string(column);
            }
            throw LoadicatorError(location + ": unknown column '" + *name + "'; the columns are " + known);
        }
        if (std::find(header.begin(), name, *name) != name) {
            throw LoadicatorError(location + ": column '" + *name + "' is named twice");
        }
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
