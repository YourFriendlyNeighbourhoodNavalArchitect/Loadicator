#include "commandLine.h"

#include "loadicator/text/numberParsing.h"

#include <cmath>
#include <format>

namespace loadicator::app {

namespace {

constexpr int bookConditionCount = 31;

/// "3" or "03" ⇒ <data>/conditions/cond03.csv; anything else is taken as a path.
[[nodiscard]] std::filesystem::path resolveCondition(std::string_view argument,
                                                     const std::filesystem::path& dataDirectory) {
    const auto number = text::parseNumber(argument);
    if (number && *number >= 1 && *number <= bookConditionCount && std::floor(*number) == *number &&
        argument.find('.') == std::string_view::npos) {
        return dataDirectory / "conditions" / std::format("cond{:02}.csv", static_cast<int>(*number));
    }
    return std::filesystem::path(argument);
}

} // namespace

std::string usage() {
    return "Usage: loadicator [options] <condition>\n"
           "\n"
           "  <condition>          loading-condition CSV file, or a book condition number (1-31)\n"
           "\n"
           "Options:\n"
           "  --method <name>      freeTrim (default) or firstOrder\n"
           "  --data-dir <dir>     ship data directory (default: the repository's data/)\n"
           "  --output <file>      write the report to a file instead of standard output\n"
           "  --help               show this text\n";
}

ParseResult parseCommandLine(std::span<const std::string_view> arguments,
                             const std::filesystem::path& defaultDataDirectory) {
    Options options{.dataDirectory = defaultDataDirectory};
    std::optional<std::string_view> conditionArgument;

    for (std::size_t index = 0; index < arguments.size(); ++index) {
        const std::string_view argument = arguments[index];
        const auto nextValue = [&]() -> std::optional<std::string_view> {
            if (index + 1 >= arguments.size()) {
                return std::nullopt;
            }
            return arguments[++index];
        };

        if (argument == "--help" || argument == "-h") {
            return HelpRequested{};
        }
        if (argument == "--method" || argument == "--data-dir" || argument == "--output") {
            const auto value = nextValue();
            if (!value) {
                return std::format("option {} needs a value", argument);
            }
            if (argument == "--method") {
                if (*value == "freeTrim") {
                    options.method = EquilibriumMethod::freeTrim;
                } else if (*value == "firstOrder") {
                    options.method = EquilibriumMethod::firstOrder;
                } else {
                    return std::format("unknown method '{}' (use freeTrim or firstOrder)", *value);
                }
            } else if (argument == "--data-dir") {
                options.dataDirectory = std::filesystem::path(*value);
            } else {
                options.outputFile = std::filesystem::path(*value);
            }
            continue;
        }
        if (argument.starts_with("--")) {
            return std::format("unknown option '{}'", argument);
        }
        if (conditionArgument) {
            return std::string("only one loading condition may be given");
        }
        conditionArgument = argument;
    }

    if (!conditionArgument) {
        return std::string("no loading condition given");
    }
    options.condition = resolveCondition(*conditionArgument, options.dataDirectory);
    return options;
}

} // namespace loadicator::app
