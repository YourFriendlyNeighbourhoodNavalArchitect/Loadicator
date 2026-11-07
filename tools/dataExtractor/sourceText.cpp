#include "sourceText.h"

#include "loadicator/core/error.h"
#include "loadicator/text/numberParsing.h"

#include <fstream>
#include <iterator>

namespace loadicator::extractor {

SourceText SourceText::readFile(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw LoadicatorError("cannot open '" + path.string() + "'");
    }
    const std::string content{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};

    SourceText source;
    source.sourceName = path.filename().string();
    SourceLine current{1, {}};
    for (const char character : content) {
        if (character == '\n') {
            source.lineList.push_back(std::move(current));
            current = SourceLine{source.lineList.size() + 1, {}};
            continue;
        }
        const auto byte = static_cast<unsigned char>(character);
        current.text.push_back(byte >= 0x20 && byte < 0x7F ? character : ' ');
    }
    if (!current.text.empty()) {
        source.lineList.push_back(std::move(current));
    }
    return source;
}

std::string SourceText::where(const SourceLine& line, std::string_view message) const {
    return sourceName + ":" + std::to_string(line.number) + ": " + std::string(message);
}

std::vector<std::string_view> tokenize(std::string_view text) {
    std::vector<std::string_view> tokens;
    std::size_t position = 0;
    while (position < text.size()) {
        const std::size_t start = text.find_first_not_of(" \t", position);
        if (start == std::string_view::npos) {
            break;
        }
        const std::size_t end = text.find_first_of(" \t", start);
        tokens.push_back(
            text.substr(start, end == std::string_view::npos ? std::string_view::npos : end - start));
        position = end == std::string_view::npos ? text.size() : end;
    }
    return tokens;
}

std::vector<double> numericTokens(std::string_view text) {
    std::vector<double> numbers;
    for (const std::string_view token : tokenize(text)) {
        if (const auto value = text::parseNumber(token)) {
            numbers.push_back(*value);
        }
    }
    return numbers;
}

} // namespace loadicator::extractor
