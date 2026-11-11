#pragma once

#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <variant>

namespace loadicator::app {

struct Options {
    std::filesystem::path condition;
    std::filesystem::path dataDirectory;
    std::optional<std::filesystem::path> outputFile;
};

struct HelpRequested {};

using ParseResult = std::variant<Options, HelpRequested, std::string>;

[[nodiscard]] ParseResult parseCommandLine(std::span<const std::string_view> arguments,
                                           const std::filesystem::path& defaultDataDirectory);

[[nodiscard]] std::string usage();

}
