#pragma once

// A deliberately small test harness: no third-party dependency, so the project builds offline.
//
//   TEST_CASE("interpolation/brackets") { CHECK(...); CHECK_NEAR(actual, expected, tolerance); }

#include <cmath>
#include <functional>
#include <source_location>
#include <sstream>
#include <string>
#include <string_view>

namespace testing {

void registerTest(std::string_view name, std::function<void()> body);
void reportFailure(std::string_view message, std::source_location location);

struct Registrar {
    Registrar(std::string_view name, std::function<void()> body) { registerTest(name, std::move(body)); }
};

inline void check(bool condition, std::string_view expression,
                  std::source_location location = std::source_location::current()) {
    if (!condition) {
        reportFailure(std::string("CHECK(") + std::string(expression) + ")", location);
    }
}

inline void checkNear(double actual, double expected, double tolerance, std::string_view expression,
                      std::source_location location = std::source_location::current()) {
    if (!(std::abs(actual - expected) <= tolerance)) {
        std::ostringstream message;
        message << expression << ": " << actual << " differs from " << expected << " by more than "
                << tolerance;
        reportFailure(message.str(), location);
    }
}

template <typename Body>
void checkThrows(Body&& body, std::string_view expression,
                 std::source_location location = std::source_location::current()) {
    try {
        body();
    } catch (...) {
        return;
    }
    reportFailure(std::string("expected an exception from ") + std::string(expression), location);
}

} // namespace testing

#define TESTING_CONCAT_INNER(a, b) a##b
#define TESTING_CONCAT(a, b) TESTING_CONCAT_INNER(a, b)
#define TESTING_UNIQUE(prefix) TESTING_CONCAT(prefix, __LINE__)

#define TEST_CASE(name)                                                                                      \
    static void TESTING_UNIQUE(testBody)();                                                                  \
    static const ::testing::Registrar TESTING_UNIQUE(testRegistrar){name, &TESTING_UNIQUE(testBody)};        \
    static void TESTING_UNIQUE(testBody)()

#define CHECK(condition) ::testing::check(static_cast<bool>(condition), #condition)
#define CHECK_NEAR(actual, expected, tolerance)                                                              \
    ::testing::checkNear((actual), (expected), (tolerance), #actual)
#define CHECK_THROWS(expression) ::testing::checkThrows([&] { (void)(expression); }, #expression)
