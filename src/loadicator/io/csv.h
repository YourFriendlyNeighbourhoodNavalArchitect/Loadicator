#pragma once

#include <cstddef>
#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <ostream>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace loadicator::io {

/// Minimal CSV dialect used by every data file:
///   - comma separated, no quoting (fields never contain commas);
///   - "# key: value" lines carry metadata, other "#" lines are comments;
///   - the first non-comment line is the header; columns are addressed by name.
class CsvRecord {
public:
    struct Header {
        std::string sourceName;
        std::vector<std::string> columns;
    };

    CsvRecord(std::shared_ptr<const Header> sharedHeader, std::size_t sourceLine,
              std::vector<std::string> values);

    /// Raw field (trimmed); empty if the field is blank. Throws if the column does not exist.
    [[nodiscard]] std::string_view text(std::string_view column) const;
    /// Required numeric field.
    [[nodiscard]] double number(std::string_view column) const;
    /// Optional numeric field: blank ⇒ nullopt, malformed ⇒ throws.
    [[nodiscard]] std::optional<double> optionalNumber(std::string_view column) const;

    /// "file:line", for error messages.
    [[nodiscard]] std::string location() const;

private:
    [[nodiscard]] std::size_t columnIndex(std::string_view column) const;

    std::shared_ptr<const Header> header;
    std::size_t lineNumber;
    std::vector<std::string> fields;
};

class CsvDocument {
public:
    [[nodiscard]] static CsvDocument readFile(const std::filesystem::path& path);
    [[nodiscard]] static CsvDocument parse(std::string_view content, std::string sourceName);

    [[nodiscard]] const std::string& sourceName() const { return header->sourceName; }
    [[nodiscard]] std::optional<std::string> metadata(std::string_view key) const;
    [[nodiscard]] bool hasColumn(std::string_view column) const;
    [[nodiscard]] std::span<const CsvRecord> records() const { return recordList; }

    /// Throws unless every listed column is present; call once before reading records.
    void requireColumns(std::initializer_list<std::string_view> columns) const;

private:
    std::shared_ptr<CsvRecord::Header> header;
    std::map<std::string, std::string, std::less<>> metadataEntries;
    std::vector<CsvRecord> recordList;
};

/// Writes the same dialect. Numbers are written with a fixed number of decimals so that
/// regenerated data files diff cleanly.
class CsvWriter {
public:
    explicit CsvWriter(std::ostream& stream);

    void metadata(std::string_view key, std::string_view value);
    void comment(std::string_view text);
    void header(std::initializer_list<std::string_view> columns);
    void row(std::span<const std::string> fields);

private:
    void writeFields(std::span<const std::string> fields);

    std::ostream& output;
    std::size_t columnCount = 0;
};

/// Fixed-point formatting with "-0.00" normalised to "0.00".
[[nodiscard]] std::string formatNumber(double value, int decimals);

} // namespace loadicator::io
