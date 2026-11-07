#pragma once

#include "loadicator/calc/equilibrium.h"

#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <variant>

namespace loadicator::app {

struct Options {
    std::filesystem::path condition;
    std::filesystem::path dataDirectory;
    std::optional<std::filesystem::path> outputFile;
    EquilibriumMethod method = EquilibriumMethod::freeTrim;
};

struct HelpRequested {};

/// Either parsed options, a request for help, or an error message.
using ParseResult = std::variant<Options, HelpRequested, std::string>;

[[nodiscard]] ParseResult parseCommandLine(std::span<const std::string_view> arguments,
                                           const std::filesystem::path& defaultDataDirectory);

[[nodiscard]] std::string usage();

} // namespace loadicator::app
