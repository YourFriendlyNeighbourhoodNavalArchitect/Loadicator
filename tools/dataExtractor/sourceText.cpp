#include "sourceText.h"

#include "loadicator/io/textFile.h"
#include "loadicator/util/text.h"

namespace loadicator::extractor {

namespace {

constexpr std::string_view digits = "0123456789";

}

SourceText SourceText::readFile(const std::filesystem::path& path) {
    std::string content = io::readTextFile(path);
    for (char& character : content) {
        const auto byte = static_cast<unsigned char>(character);
        if (character != '\n' && (byte < 0x20 || byte >= 0x7F)) {
            character = ' ';
        }
    }

    SourceText source;
    source.fileName = path.filename().string();
    for (const std::string_view line : text::split(content, '\n')) {
        source.lineList.push_back(SourceLine{source.lineList.size() + 1, std::string(text::trim(line))});
    }
    return source;
}

LoadicatorError SourceText::error(std::string_view message) const {
    return LoadicatorError(fileName + ": " + std::string(message));
}

std::string SourceText::location(const SourceLine& line) const {
    return fileName + ":" + std::to_string(line.number);
}

LoadicatorError SourceText::error(const SourceLine& line, std::string_view message) const {
    return LoadicatorError(location(line) + ": " + std::string(message));
}

std::vector<std::string_view> tokenize(std::string_view text) {
    std::vector<std::string_view> tokens;
    for (std::size_t start = text.find_first_not_of(' '); start != std::string_view::npos;) {
        const std::size_t end = std::min(text.find(' ', start), text.size());
        tokens.push_back(text.substr(start, end - start));
        start = text.find_first_not_of(' ', end);
    }
    return tokens;
}

std::vector<double> parseNumbers(std::string_view text) {
    std::vector<double> numbers;
    for (std::size_t start = 0; start < text.size();) {
        std::size_t end = start + (text[start] == '-' || text[start] == '+' ? 1 : 0);
        end = std::min(text.find_first_not_of(digits, end), text.size());
        if (end < text.size() && text[end] == '.') {
            end = std::min(text.find_first_not_of(digits, end + 1), text.size());
        }
        if (const auto number = text::parseNumber(text.substr(start, end - start))) {
            numbers.push_back(*number);
            start = end;
        } else {
            ++start;
        }
    }
    return numbers;
}

}
