#include "loadicator/io/csv.h"

#include "loadicator/core/error.h"
#include "loadicator/text/numberParsing.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <utility>

namespace loadicator::io {

CsvRecord::CsvRecord(std::shared_ptr<const Header> sharedHeader, std::size_t sourceLine,
                     std::vector<std::string> values)
    : header(std::move(sharedHeader)), lineNumber(sourceLine), fields(std::move(values)) {
}

std::size_t CsvRecord::columnIndex(std::string_view column) const {
    const auto& columns = header->columns;
    const auto found = std::find(columns.begin(), columns.end(), column);
    if (found == columns.end()) {
        throw LoadicatorError(header->sourceName + ": missing column '" + std::string(column) + "'");
    }
    return static_cast<std::size_t>(found - columns.begin());
}

std::string_view CsvRecord::text(std::string_view column) const {
    return text::trim(fields[columnIndex(column)]);
}

double CsvRecord::number(std::string_view column) const {
    const auto value = optionalNumber(column);
    if (!value) {
        throw LoadicatorError(location() + ": column '" + std::string(column) + "' is required");
    }
    return *value;
}

std::optional<double> CsvRecord::optionalNumber(std::string_view column) const {
    const std::string_view field = text(column);
    if (field.empty()) {
        return std::nullopt;
    }
    const auto value = text::parseNumber(field);
    if (!value) {
        throw LoadicatorError(location() + ": column '" + std::string(column) + "' is not a number: '" +
                              std::string(field) + "'");
    }
    return value;
}

std::string CsvRecord::location() const {
    return header->sourceName + ":" + std::to_string(lineNumber);
}

CsvDocument CsvDocument::readFile(const std::filesystem::path& path) {
    const std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw LoadicatorError("cannot open '" + path.string() + "'");
    }
    std::ostringstream content;
    content << file.rdbuf();
    return parse(content.str(), path.string());
}

CsvDocument CsvDocument::parse(std::string_view content, std::string sourceName) {
    CsvDocument document;
    document.header = std::make_shared<CsvRecord::Header>();
    document.header->sourceName = std::move(sourceName);

    std::size_t lineNumber = 0;
    for (const std::string_view rawLine : text::split(content, '\n')) {
        ++lineNumber;
        const std::string_view line = text::trim(rawLine);
        if (line.empty()) {
            continue;
        }
        if (line.front() == '#') {
            const std::string_view body = text::trim(line.substr(1));
            const std::size_t colon = body.find(':');
            if (colon != std::string_view::npos &&
                body.substr(0, colon).find(' ') == std::string_view::npos) {
                document.metadataEntries.emplace(std::string(body.substr(0, colon)),
                                                 std::string(text::trim(body.substr(colon + 1))));
            }
            continue;
        }

        std::vector<std::string> fields;
        for (const std::string_view field : text::split(line, ',')) {
            fields.emplace_back(text::trim(field));
        }

        if (document.header->columns.empty()) {
            document.header->columns = std::move(fields);
            continue;
        }
        if (fields.size() != document.header->columns.size()) {
            throw LoadicatorError(document.header->sourceName + ":" + std::to_string(lineNumber) +
                                  ": expected " + std::to_string(document.header->columns.size()) +
                                  " fields, found " + std::to_string(fields.size()));
        }
        document.recordList.emplace_back(document.header, lineNumber, std::move(fields));
    }

    if (document.header->columns.empty()) {
        throw LoadicatorError(document.header->sourceName + ": no header row");
    }
    return document;
}

std::optional<std::string> CsvDocument::metadata(std::string_view key) const {
    const auto found = metadataEntries.find(key);
    return found == metadataEntries.end() ? std::nullopt : std::optional<std::string>(found->second);
}

bool CsvDocument::hasColumn(std::string_view column) const {
    const auto& columns = header->columns;
    return std::find(columns.begin(), columns.end(), column) != columns.end();
}

void CsvDocument::requireColumns(std::initializer_list<std::string_view> columns) const {
    for (const std::string_view column : columns) {
        if (!hasColumn(column)) {
            throw LoadicatorError(sourceName() + ": missing column '" + std::string(column) + "'");
        }
    }
}

CsvWriter::CsvWriter(std::ostream& stream) : output(stream) {
}

void CsvWriter::metadata(std::string_view key, std::string_view value) {
    output << "# " << key << ": " << value << '\n';
}

void CsvWriter::comment(std::string_view text) {
    output << "# " << text << '\n';
}

void CsvWriter::header(std::initializer_list<std::string_view> columns) {
    const std::vector<std::string> names(columns.begin(), columns.end());
    columnCount = names.size();
    writeFields(names);
}

void CsvWriter::row(std::span<const std::string> fields) {
    if (fields.size() != columnCount) {
        throw LoadicatorError("CSV row has " + std::to_string(fields.size()) + " fields, header has " +
                              std::to_string(columnCount));
    }
    writeFields(fields);
}

void CsvWriter::writeFields(std::span<const std::string> fields) {
    for (std::size_t index = 0; index < fields.size(); ++index) {
        if (fields[index].find_first_of(",\n") != std::string::npos) {
            throw LoadicatorError("CSV field may not contain a comma or newline: '" + fields[index] + "'");
        }
        output << (index == 0 ? "" : ",") << fields[index];
    }
    output << '\n';
}

std::string formatNumber(double value, int decimals) {
    std::ostringstream stream;
    stream.imbue(std::locale::classic());
    stream << std::fixed << std::setprecision(decimals) << value;
    std::string formatted = stream.str();
    if (formatted.front() == '-' && formatted.find_first_not_of("-0.") == std::string::npos) {
        formatted.erase(0, 1);
    }
    return formatted;
}

} // namespace loadicator::io
