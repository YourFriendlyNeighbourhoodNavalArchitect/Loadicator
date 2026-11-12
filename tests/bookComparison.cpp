#include "loadicator/book/bookResults.h"
#include "loadicator/calc/analysis.h"
#include "loadicator/io/csv.h"
#include "loadicator/io/dataLoader.h"
#include "loadicator/io/schema.h"
#include "loadicator/util/error.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <exception>
#include <filesystem>
#include <format>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

using namespace loadicator;
namespace schema = io::schema;

struct Metric {
    std::string_view name;
    std::string_view unit;
    std::string_view bookColumn;
    double (*computed)(const Analysis&);
    // The test fails when the mean absolute deviation over the conditions of the book exceeds this.
    double meanLimit;
};

// A deviation beyond this multiple of the mean limit of its metric is listed as an outlier, for information.
constexpr double outlierFactor = 3.0;

constexpr std::array metrics{
    Metric{"Displacement", "t", book::column::displacement,
           [](const Analysis& a) { return a.mass.displacement.value(); }, 0.5},
    Metric{"LCG", "m", book::column::lcg, [](const Analysis& a) { return a.mass.centreOfGravity.x.value(); },
           0.02},
    Metric{"TCG", "m", book::column::tcg, [](const Analysis& a) { return a.mass.centreOfGravity.y.value(); },
           0.05},
    Metric{"KG", "m", book::column::kg, [](const Analysis& a) { return a.mass.centreOfGravity.z.value(); },
           0.01},
    Metric{"Draught midships", "m", book::column::draught,
           [](const Analysis& a) { return a.floatingPosition.draughtMidships.value(); }, 0.02},
    Metric{"Draught aft", "m", book::column::draughtAft,
           [](const Analysis& a) { return a.floatingPosition.draughtAft.value(); }, 0.02},
    Metric{"Draught forward", "m", book::column::draughtForward,
           [](const Analysis& a) { return a.floatingPosition.draughtForward.value(); }, 0.04},
    Metric{"Trim", "m", book::column::trim, [](const Analysis& a) { return a.floatingPosition.trim.value(); },
           0.05},
    Metric{"KMT", "m", book::column::kmt, [](const Analysis& a) { return a.floatingPosition.kmt.value(); },
           0.05},
    Metric{"GM solid", "m", book::column::gmSolid,
           [](const Analysis& a) { return a.stability.gmSolid.value(); }, 0.05},
    Metric{"GM fluid", "m", book::column::gm, [](const Analysis& a) { return a.stability.gmFluid.value(); },
           0.05},
    Metric{"Heel", "deg", book::column::heel,
           [](const Analysis& a) {
               if (!a.stability.heel) {
                   throw LoadicatorError("the condition is unstable, so it has no heel to compare");
               }
               return a.stability.heel->degrees();
           },
           0.4},
};

struct Outlier {
    std::string condition;
    double deviation = 0.0;
};

struct Statistics {
    double sum = 0.0;
    double absoluteSum = 0.0;
    double worst = 0.0;
    std::string worstCondition;
    std::vector<Outlier> outliers;

    void add(const std::string& condition, double deviation, double outlierLimit) {
        sum += deviation;
        absoluteSum += std::abs(deviation);
        if (std::abs(deviation) >= std::abs(worst)) {
            worst = deviation;
            worstCondition = condition;
        }
        if (std::abs(deviation) > outlierLimit) {
            outliers.push_back(Outlier{condition, deviation});
        }
    }
};

int run(const std::filesystem::path& dataDirectory) {
    const Ship ship = io::loadShip(dataDirectory);
    const io::CsvDocument bookResults =
        io::CsvDocument::readFile(dataDirectory / book::resultsFile, book::columns);
    const io::CsvColumn condition = bookResults.column(book::column::condition);
    std::array<io::CsvColumn, metrics.size()> columns;
    for (std::size_t index = 0; index < metrics.size(); ++index) {
        columns[index] = bookResults.column(metrics[index].bookColumn);
    }

    std::array<Statistics, metrics.size()> statistics;
    for (const io::CsvRow expected : bookResults.rows()) {
        const std::string name{expected.text(condition)};
        withContext(name, [&] {
            const Analysis analysis = analyzeLoadingCondition(
                ship, io::loadLoadingCondition(dataDirectory / schema::files::conditions /
                                               (name + std::string(schema::files::extension))));
            for (std::size_t index = 0; index < metrics.size(); ++index) {
                const Metric& metric = metrics[index];
                statistics[index].add(name, metric.computed(analysis) - expected.number(columns[index]),
                                      outlierFactor * metric.meanLimit);
            }
        });
    }

    const auto conditionCount = static_cast<double>(bookResults.rowCount());
    std::cout << std::format("Deviation from the trim and stability book over {} loading conditions\n",
                             bookResults.rowCount())
              << "(computed minus book; the test fails when the mean absolute deviation of a metric "
                 "exceeds its limit)\n\n"
              << std::format("{:<18}{:<5}{:>10}{:>8}{:>10}{:>10}  {:<8}{:>9}\n", "Metric", "Unit",
                             "Mean abs", "Limit", "Bias", "Worst", "in", "Outliers");
    bool passed = true;
    for (std::size_t index = 0; index < metrics.size(); ++index) {
        const Metric& metric = metrics[index];
        const Statistics& metricStatistics = statistics[index];
        const double meanAbsolute = metricStatistics.absoluteSum / conditionCount;
        const bool metricPasses = meanAbsolute <= metric.meanLimit;
        passed = passed && metricPasses;
        std::cout << std::format("{:<18}{:<5}{:>10.4f}{:>8.2f}{:>+10.4f}{:>+10.4f}  {:<8}{:>9}{}\n",
                                 metric.name, metric.unit, meanAbsolute, metric.meanLimit,
                                 metricStatistics.sum / conditionCount, metricStatistics.worst,
                                 metricStatistics.worstCondition, metricStatistics.outliers.size(),
                                 metricPasses ? "" : "  FAILED");
    }

    std::cout << std::format(
        "\nOutliers, for information: deviations beyond {} times the limit of their metric\n", outlierFactor);
    for (std::size_t index = 0; index < metrics.size(); ++index) {
        for (const Outlier& outlier : statistics[index].outliers) {
            std::cout << std::format("  {:<10}{:<18}{:>+10.4f} {}\n", outlier.condition, metrics[index].name,
                                     outlier.deviation, metrics[index].unit);
        }
    }

    std::cout << '\n' << (passed ? "PASSED" : "FAILED") << '\n';
    return passed ? 0 : 1;
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
