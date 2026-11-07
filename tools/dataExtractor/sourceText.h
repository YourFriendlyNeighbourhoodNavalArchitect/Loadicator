#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace loadicator::extractor {

/// One line of a source document with its 1-based line number, for error messages.
struct SourceLine {
    std::size_t number = 0;
    std::string text;
};

/// A source text file, split into lines. Bytes outside printable ASCII (CR, form feeds,
/// the NEL characters in the sounding tables, ...) are replaced by spaces.
class SourceText {
public:
    [[nodiscard]] static SourceText readFile(const std::filesystem::path& path);

    [[nodiscard]] const std::string& name() const { return sourceName; }
    [[nodiscard]] const std::vector<SourceLine>& lines() const { return lineList; }

    /// "name:line: message"
    [[nodiscard]] std::string where(const SourceLine& line, std::string_view message) const;

private:
    std::string sourceName;
    std::vector<SourceLine> lineList;
};

/// Splits on runs of whitespace.
[[nodiscard]] std::vector<std::string_view> tokenize(std::string_view text);

/// Whole-token numbers only: "TA 9.003 m GM0 11.26 m" ⇒ {9.003, 11.26}.
[[nodiscard]] std::vector<double> numericTokens(std::string_view text);

} // namespace loadicator::extractor
