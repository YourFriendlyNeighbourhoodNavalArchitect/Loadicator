#include "io/csv.h"

#include "util/error.h"
#include "util/text.h"

#include <algorithm>
#include <fstream>
#include <iterator>

namespace loadicator::io {

std::string readTextFile(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw LoadicatorError("cannot open '" + path.string() + "'");
    }
    std::string content{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
    // Text editors and spreadsheet programs may begin UTF-8 text with a byte order mark.
    constexpr std::string_view byteOrderMark = "\xEF\xBB\xBF";
    if (content.starts_with(byteOrderMark)) {
        content.erase(0, byteOrderMark.size());
    }
    return content;
}

std::string_view CsvRow::text(std::string_view column) const {
    const auto field = fields.find(column);
    return field == fields.end() ? std::string_view() : std::string_view(field->second);
}

double CsvRow::number(std::string_view column) const {
    const auto value = optionalNumber(column);
    if (!value) {
        throw LoadicatorError(rowLocation + ": column '" + std::string(column) + "' is required");
    }
    return *value;
}

std::optional<double> CsvRow::optionalNumber(std::string_view column) const {
    const std::string_view field = text(column);
    if (field.empty()) {
        return std::nullopt;
    }
    const auto value = text::parseNumber(field);
    if (!value) {
        throw LoadicatorError(rowLocation + ": column '" + std::string(column) + "' is not a number: '" +
                              std::string(field) + "'");
    }
    return value;
}

std::vector<CsvRow> readCsv(const std::filesystem::path& path,
                            std::span<const std::string_view> knownColumns) {
    const std::string content = readTextFile(path);
    std::vector<std::string> header;
    std::vector<CsvRow> rows;
    std::size_t lineNumber = 0;
    for (const std::string_view line : text::split(content, '\n')) {
        ++lineNumber;
        if (text::trim(line).empty()) {
            continue;
        }
        const std::string location = path.string() + ":" + std::to_string(lineNumber);
        std::vector<std::string> fields;
        for (const std::string_view field : text::split(line, ',')) {
            fields.emplace_back(text::trim(field));
        }

        if (header.empty()) {
            for (const std::string& name : fields) {
                if (std::ranges::find(knownColumns, name) == knownColumns.end()) {
                    throw LoadicatorError(location + ": unknown column '" + name + "'");
                }
                if (std::ranges::count(fields, name) > 1) {
                    throw LoadicatorError(location + ": column '" + name + "' is named twice");
                }
            }
            header = std::move(fields);
            continue;
        }
        if (fields.size() != header.size()) {
            throw LoadicatorError(location + ": expected " + std::to_string(header.size()) +
                                  " fields, found " + std::to_string(fields.size()));
        }
        std::map<std::string, std::string, std::less<>> row;
        for (std::size_t index = 0; index < fields.size(); ++index) {
            row.emplace(header[index], std::move(fields[index]));
        }
        rows.emplace_back(location, std::move(row));
    }
    return rows;
}

}
