#include "commandLine.h"

#include "io/schema.h"

#include <charconv>
#include <format>

namespace loadicator::app {

namespace {

namespace schema = io::schema;

[[nodiscard]] std::filesystem::path resolveCondition(std::string_view argument,
                                                     const std::filesystem::path& dataDirectory) {
    int number = 0;
    const char* const end = argument.data() + argument.size();
    if (const auto parsed = std::from_chars(argument.data(), end, number);
        parsed.ec == std::errc{} && parsed.ptr == end) {
        const std::filesystem::path bookCondition =
            dataDirectory / schema::files::conditions /
            (schema::bookConditionKey(number) + std::string(schema::files::extension));
        if (std::filesystem::exists(bookCondition)) {
            return bookCondition;
        }
    }
    return std::filesystem::path(argument);
}

}

std::string usage() {
    return "Usage: loadicator [options] <condition>\n"
           "\n"
           "  <condition>          loading-condition CSV file, or the number of a condition of the book\n"
           "\n"
           "Options:\n"
           "  --data-dir <dir>     ship data directory (default: $LOADICATOR_DATA_DIR, or ./data)\n"
           "  --output <file>      write the report to a file instead of standard output\n"
           "  --help               show this text\n";
}

ParseResult parseCommandLine(std::span<const std::string_view> arguments,
                             const std::filesystem::path& defaultDataDirectory) {
    Options options{.dataDirectory = defaultDataDirectory};
    std::optional<std::string_view> condition;

    for (std::size_t index = 0; index < arguments.size(); ++index) {
        const std::string_view argument = arguments[index];
        if (argument == "--help" || argument == "-h") {
            return HelpRequested{};
        }
        if (argument == "--data-dir" || argument == "--output") {
            if (++index == arguments.size()) {
                return std::format("option {} needs a value", argument);
            }
            (argument == "--data-dir" ? options.dataDirectory : options.outputFile.emplace()) =
                std::filesystem::path(arguments[index]);
        } else if (argument.starts_with("--")) {
            return std::format("unknown option '{}'", argument);
        } else if (condition) {
            return std::string("only one loading condition may be given");
        } else {
            condition = argument;
        }
    }

    if (!condition) {
        return std::string("no loading condition given");
    }
    options.condition = resolveCondition(*condition, options.dataDirectory);
    return options;
}

}
