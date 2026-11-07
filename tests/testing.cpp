#include "testing.h"

#include <exception>
#include <iostream>
#include <utility>
#include <vector>

namespace testing {

namespace {

struct TestCase {
    std::string name;
    std::function<void()> body;
};

std::vector<TestCase>& registry() {
    static std::vector<TestCase> tests;
    return tests;
}

int failuresInCurrentTest = 0;

} // namespace

void registerTest(std::string_view name, std::function<void()> body) {
    registry().push_back({std::string(name), std::move(body)});
}

void reportFailure(std::string_view message, std::source_location location) {
    ++failuresInCurrentTest;
    std::cerr << "    " << location.file_name() << ":" << location.line() << ": " << message << '\n';
}

} // namespace testing

/// Usage: loadicatorTests [name-prefix]   (runs every test whose name starts with the prefix)
int main(int argumentCount, char** arguments) {
    const std::string_view prefix = argumentCount > 1 ? arguments[1] : "";
    int failedTests = 0;
    int ranTests = 0;
    for (const auto& test : testing::registry()) {
        if (!test.name.starts_with(prefix)) {
            continue;
        }
        ++ranTests;
        testing::failuresInCurrentTest = 0;
        try {
            test.body();
        } catch (const std::exception& error) {
            testing::reportFailure(std::string("unexpected exception: ") + error.what(),
                                   std::source_location::current());
        }
        const bool passed = testing::failuresInCurrentTest == 0;
        failedTests += passed ? 0 : 1;
        std::cout << (passed ? "[pass] " : "[FAIL] ") << test.name << '\n';
    }
    std::cout << ranTests - failedTests << " of " << ranTests << " tests passed\n";
    return (failedTests == 0 && ranTests > 0) ? 0 : 1;
}
