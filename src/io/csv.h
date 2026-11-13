#pragma once

#include <filesystem>
#include <functional>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace loadicator::io {

// The content of a text file, without a leading UTF-8 byte order mark.
[[nodiscard]] std::string readTextFile(const std::filesystem::path& path);

class CsvRow {
public:
    CsvRow(std::string location, std::map<std::string, std::string, std::less<>> rowFields)
        : rowLocation(std::move(location)), fields(std::move(rowFields)) {}

    // The file and line of the row, for messages.
    [[nodiscard]] const std::string& location() const { return rowLocation; }

    // Empty if the file has no such column.
    [[nodiscard]] std::string_view text(std::string_view column) const;
    [[nodiscard]] double number(std::string_view column) const;
    [[nodiscard]] std::optional<double> optionalNumber(std::string_view column) const;

private:
    std::string rowLocation;
    std::map<std::string, std::string, std::less<>> fields;
};

// The rows of a CSV file, whose columns are addressed by the names in its header, in any order. Fails if the
// header names a column twice, or one that is not among the known columns.
[[nodiscard]] std::vector<CsvRow> readCsv(const std::filesystem::path& path,
                                          std::span<const std::string_view> knownColumns);

}
