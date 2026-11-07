#include "../testing.h"
#include "loadicator/text/numberParsing.h"

#include <cmath>
#include <string_view>

using namespace loadicator::text;

TEST_CASE("text/parseNumberAcceptsSourceForms") {
    const auto valueOf = [](std::string_view token) {
        return parseNumber(token).value_or(std::nan(""));
    };
    CHECK_NEAR(valueOf("12"), 12.0, 0.0);
    CHECK_NEAR(valueOf("-3.5"), -3.5, 0.0);
    CHECK_NEAR(valueOf(".911"), 0.911, 1e-15);
    CHECK_NEAR(valueOf(" 7. "), 7.0, 0.0);
    // Audit C9: the original regex turned "-.116" into +0.116.
    CHECK_NEAR(valueOf("-.116"), -0.116, 1e-15);
}

TEST_CASE("text/parseNumberRejectsNonNumbers") {
    CHECK(!parseNumber(""));
    CHECK(!parseNumber("-"));
    CHECK(!parseNumber("."));
    CHECK(!parseNumber("abc"));
    CHECK(!parseNumber("1.2.3"));
    CHECK(!parseNumber("GM0"));
    CHECK(!parseNumber("12m"));
}

TEST_CASE("text/parseNumbersSplitsGluedValues") {
    const auto values = parseNumbers("51.30-13.52 5.34 -.425|");
    CHECK(values.size() == 4);
    CHECK_NEAR(values[0], 51.30, 1e-12);
    CHECK_NEAR(values[1], -13.52, 1e-12);
    CHECK_NEAR(values[2], 5.34, 1e-12);
    CHECK_NEAR(values[3], -0.425, 1e-12);
}

TEST_CASE("text/trimAndSplit") {
    CHECK(trim("  a b \r\n") == "a b");
    CHECK(trim(" \t ").empty());
    const auto fields = split("a,,b", ',');
    CHECK(fields.size() == 3);
    CHECK(fields[1].empty());
    CHECK(fields[2] == "b");
}
