#pragma once

#include "util/error.h"

#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace loadicator::extractor {

struct SourceLine {
    std::size_t number = 0;
    std::string text;
};

class SourceText {
public:
    [[nodiscard]] static SourceText readFile(const std::filesystem::path& path);

    [[nodiscard]] const std::string& name() const { return fileName; }
    [[nodiscard]] const std::vector<SourceLine>& lines() const { return lineList; }

    [[nodiscard]] LoadicatorError error(std::string_view message) const;
    [[nodiscard]] LoadicatorError error(const SourceLine& line, std::string_view message) const;

private:
    std::string fileName;
    std::vector<SourceLine> lineList;
};

[[nodiscard]] std::vector<std::string_view> tokenize(std::string_view text);

[[nodiscard]] std::vector<double> parseNumbers(std::string_view text);

}
