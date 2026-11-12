#pragma once

#include <cstddef>
#include <filesystem>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

namespace loadicator::io {

class CsvColumn {
public:
    [[nodiscard]] std::string_view name() const { return columnName; }

private:
    friend class CsvDocument;
    friend class CsvRow;

    std::string_view columnName;
    std::optional<std::size_t> index;
};

class CsvDocument;

class CsvRow {
public:
    CsvRow(const CsvDocument& owner, std::size_t rowIndex) : document(&owner), index(rowIndex) {}

    [[nodiscard]] std::string_view text(const CsvColumn& column) const;
    [[nodiscard]] double number(const CsvColumn& column) const;
    [[nodiscard]] std::optional<double> optionalNumber(const CsvColumn& column) const;

    [[nodiscard]] std::string location() const;

private:
    const CsvDocument* document;
    std::size_t index;
};

class CsvDocument {
public:
    [[nodiscard]] static CsvDocument readFile(const std::filesystem::path& path);

    [[nodiscard]] const std::string& sourceName() const { return source; }

    [[nodiscard]] CsvColumn column(std::string_view name) const;
    [[nodiscard]] CsvColumn optionalColumn(std::string_view name) const;

    [[nodiscard]] std::size_t rowCount() const { return lines.size(); }
    [[nodiscard]] auto rows() const {
        return std::views::iota(std::size_t{0}, lines.size()) |
               std::views::transform([this](std::size_t index) { return CsvRow(*this, index); });
    }

private:
    friend class CsvRow;

    struct Line {
        std::size_t number = 0;
        std::vector<std::string> fields;
    };

    CsvDocument(std::string_view content, std::string sourceName);

    std::string source;
    std::vector<std::string> header;
    std::vector<Line> lines;
};

}
