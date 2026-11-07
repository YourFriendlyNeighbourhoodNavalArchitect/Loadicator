#include "../testing.h"
#include "loadicator/io/csv.h"

#include <sstream>
#include <string>
#include <vector>

using namespace loadicator::io;

TEST_CASE("csv/parsesMetadataHeaderAndRecords") {
    const auto document = CsvDocument::parse("# id: R2.01\n# a free comment: with spaces\n"
                                             "volume, lcg ,note\n\n1.5,2,x\n3,,y\n",
                                             "inline");
    CHECK(document.metadata("id") == std::optional<std::string>("R2.01"));
    CHECK(!document.metadata("a free comment"));
    CHECK(document.records().size() == 2);
    CHECK_NEAR(document.records()[0].number("volume"), 1.5, 0.0);
    CHECK(document.records()[0].text("note") == "x");
    CHECK(!document.records()[1].optionalNumber("lcg"));
    CHECK(document.records()[1].location() == "inline:6");
}

TEST_CASE("csv/reportsBadInput") {
    CHECK_THROWS(CsvDocument::parse("", "empty"));
    CHECK_THROWS(CsvDocument::parse("a,b\n1\n", "short row"));
    const auto document = CsvDocument::parse("a\nx\n", "text");
    CHECK_THROWS(document.records()[0].number("a"));
    CHECK_THROWS(document.records()[0].number("missing"));
    CHECK_THROWS(document.requireColumns({"a", "b"}));
}

TEST_CASE("csv/writerRoundTrip") {
    std::ostringstream stream;
    CsvWriter writer(stream);
    writer.metadata("id", "R1.1");
    writer.header({"x", "y"});
    const std::vector<std::string> row{formatNumber(-0.0001, 2), formatNumber(2.345, 1)};
    writer.row(row);
    CHECK(stream.str() == "# id: R1.1\nx,y\n0.00,2.3\n");
    const std::vector<std::string> bad{"a,b", "c"};
    CHECK_THROWS(writer.row(bad));
}
