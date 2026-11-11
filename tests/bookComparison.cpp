#include "calc/analysis.h"
#include "io/csv.h"
#include "io/dataLoader.h"
#include "io/schema.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <exception>
#include <filesystem>
#include <format>
#include <iostream>
#include <string>
#include <string_view>

namespace {

using namespace loadicator;
namespace schema = io::schema;

struct Metric {
    std::string_view name;
    std::string_view unit;
    std::string_view bookColumn;
    double (*computed)(const Analysis&);
    double meanLimit;
};

constexpr std::array metrics{
    Metric{"Displacement", "t", schema::bookResults::displacement,
           [](const Analysis& a) { return a.mass.displacement.value(); }, 0.5},
    Metric{"LCG", "m", schema::bookResults::lcg,
           [](const Analysis& a) { return a.mass.centreOfGravity.x.value(); }, 0.02},
    Metric{"TCG", "m", schema::bookResults::tcg,
           [](const Analysis& a) { return a.mass.centreOfGravity.y.value(); }, 0.05},
    Metric{"KG", "m", schema::bookResults::kg,
           [](const Analysis& a) { return a.mass.centreOfGravity.z.value(); }, 0.01},
    Metric{"Draught midships", "m", schema::bookResults::draught,
           [](const Analysis& a) { return a.floatingPosition.draughtMidships.value(); }, 0.02},
    Metric{"Draught aft", "m", schema::bookResults::draughtAft,
           [](const Analysis& a) { return a.floatingPosition.draughtAft.value(); }, 0.02},
    Metric{"Draught forward", "m", schema::bookResults::draughtForward,
           [](const Analysis& a) { return a.floatingPosition.draughtForward.value(); }, 0.04},
    Metric{"Trim", "m", schema::bookResults::trim,
           [](const Analysis& a) { return a.floatingPosition.trim.value(); }, 0.05},
    Metric{"KMT", "m", schema::bookResults::kmt,
           [](const Analysis& a) { return a.floatingPosition.kmt.value(); }, 0.05},
    Metric{"GM solid", "m", schema::bookResults::gmSolid,
           [](const Analysis& a) { return a.stability.gmSolid.value(); }, 0.05},
    Metric{"GM fluid", "m", schema::bookResults::gm,
           [](const Analysis& a) { return a.stability.gmFluid.value(); }, 0.05},
    Metric{"Heel", "deg", schema::bookResults::heel,
           [](const Analysis& a) { return a.stability.heel.value().degrees(); }, 0.4},
};

struct Deviation {
    double sum = 0.0;
    double absoluteSum = 0.0;
    double worst = 0.0;
    std::string worstCondition;

    void add(double deviation, const std::string& condition) {
        sum += deviation;
        absoluteSum += std::abs(deviation);
        if (std::abs(deviation) >= std::abs(worst)) {
            worst = deviation;
            worstCondition = condition;
        }
    }
};

int run(const std::filesystem::path& dataDirectory) {
    const Ship ship = io::loadShip(dataDirectory);
    const io::CsvDocument book = io::CsvDocument::readFile(dataDirectory / schema::files::bookResults);
    const io::CsvColumn condition = book.column(schema::bookResults::condition);
    std::array<io::CsvColumn, metrics.size()> columns;
    std::transform(metrics.begin(), metrics.end(), columns.begin(),
                   [&](const Metric& metric) { return book.column(metric.bookColumn); });

    std::array<Deviation, metrics.size()> deviations;
    for (const io::CsvRow expected : book.rows()) {
        const std::string name{expected.text(condition)};
        const Analysis analysis = analyzeLoadingCondition(
            ship, io::loadLoadingCondition(dataDirectory / schema::files::conditions /
                                           (name + std::string(schema::files::extension))));
        for (std::size_t index = 0; index < metrics.size(); ++index) {
            deviations[index].add(metrics[index].computed(analysis) - expected.number(columns[index]), name);
        }
    }

    const auto conditionCount = static_cast<double>(book.rowCount());
    bool withinLimits = true;
    std::cout << std::format("Deviation from the trim and stability book over {} loading conditions\n\n",
                             book.rowCount())
              << std::format("{:<18}{:<6}{:>10}{:>10}{:>10}  {:<8}{:>8}\n", "Metric", "Unit", "Mean abs",
                             "Mean", "Worst", "in", "Limit");
    for (std::size_t index = 0; index < metrics.size(); ++index) {
        const Metric& metric = metrics[index];
        const Deviation& deviation = deviations[index];
        const double meanAbsolute = deviation.absoluteSum / conditionCount;
        const bool withinLimit = meanAbsolute <= metric.meanLimit;
        withinLimits = withinLimits && withinLimit;
        std::cout << std::format("{:<18}{:<6}{:>10.4f}{:>+10.4f}{:>+10.4f}  {:<8}{:>8.2f}{}\n", metric.name,
                                 metric.unit, meanAbsolute, deviation.sum / conditionCount, deviation.worst,
                                 deviation.worstCondition, metric.meanLimit, withinLimit ? "" : "  EXCEEDED");
    }
    return withinLimits ? 0 : 1;
}

}

int main(int argumentCount, char** arguments) {
    if (argumentCount != 2) {
        std::cerr << "usage: bookComparison <dataDirectory>\n";
        return 2;
    }
    try {
        return run(arguments[1]);
    } catch (const std::exception& error) {
        std::cerr << "bookComparison: " << error.what() << '\n';
        return 1;
    }
}
