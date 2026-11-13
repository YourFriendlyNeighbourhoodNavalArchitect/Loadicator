#include "calc/analysis.h"
#include "io/csv.h"
#include "io/dataLoader.h"
#include "io/schema.h"
#include "util/error.h"

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

struct Metric {
    std::string_view name;
    std::string_view unit;
    // The column of the results of the book.
    std::string_view column;
    double (*computed)(const Analysis&);
    // The test fails when the mean absolute deviation over the conditions of the book exceeds this.
    double meanLimit;
};

// A deviation beyond this multiple of the mean limit of its metric is listed as an outlier, for information.
constexpr double outlierFactor = 3.0;

constexpr std::array metrics{
    Metric{"Displacement", "t", "displacement", [](const Analysis& a) { return a.mass.displacement.value(); },
           0.5},
    Metric{"LCG", "m", "lcg", [](const Analysis& a) { return a.mass.centreOfGravity.x.value(); }, 0.02},
    Metric{"TCG", "m", "tcg", [](const Analysis& a) { return a.mass.centreOfGravity.y.value(); }, 0.05},
    Metric{"KG", "m", "kg", [](const Analysis& a) { return a.mass.centreOfGravity.z.value(); }, 0.01},
    Metric{"Draught midships", "m", "draught",
           [](const Analysis& a) { return a.floatingPosition.draughtMidships.value(); }, 0.02},
    Metric{"Draught aft", "m", "draughtAft",
           [](const Analysis& a) { return a.floatingPosition.draughtAft.value(); }, 0.02},
    Metric{"Draught forward", "m", "draughtForward",
           [](const Analysis& a) { return a.floatingPosition.draughtForward.value(); }, 0.04},
    Metric{"Trim", "m", "trim", [](const Analysis& a) { return a.floatingPosition.trim.value(); }, 0.05},
    Metric{"KMT", "m", "kmt", [](const Analysis& a) { return a.floatingPosition.kmt.value(); }, 0.05},
    Metric{"GM solid", "m", "gmSolid", [](const Analysis& a) { return a.stability.gmSolid.value(); }, 0.05},
    Metric{"GM fluid", "m", "gm", [](const Analysis& a) { return a.stability.gmFluid.value(); }, 0.05},
    Metric{"Heel", "deg", "heel",
           [](const Analysis& a) {
               if (!a.stability.heel) {
                   throw LoadicatorError("the condition is unstable, so it has no heel to compare");
               }
               return *a.stability.heel;
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
    std::vector<std::string_view> resultColumns{"condition"};
    for (const Metric& metric : metrics) {
        resultColumns.push_back(metric.column);
    }
    const std::vector<io::CsvRow> bookResults =
        io::readCsv(dataDirectory / io::schema::bookResultsFile, resultColumns);

    std::array<Statistics, metrics.size()> statistics;
    for (const io::CsvRow& expected : bookResults) {
        const std::string name{expected.text("condition")};
        withContext(name, [&] {
            const Analysis analysis = analyzeLoadingCondition(
                ship,
                io::loadLoadingCondition(dataDirectory / io::schema::conditionDirectory / (name + ".csv")));
            for (std::size_t index = 0; index < metrics.size(); ++index) {
                const Metric& metric = metrics[index];
                statistics[index].add(name, metric.computed(analysis) - expected.number(metric.column),
                                      outlierFactor * metric.meanLimit);
            }
        });
    }

    const auto conditionCount = static_cast<double>(bookResults.size());
    std::cout << std::format("Deviation from the trim and stability book over {} loading conditions\n",
                             bookResults.size())
              << "(computed minus book; the test fails when the mean absolute deviation of a metric "
                 "exceeds its limit)\n\n"
              << std::format("{:<18}{:<5}{:>10}{:>8}{:>10}{:>10}  {}\n", "Metric", "Unit", "Mean abs",
                             "Limit", "Bias", "Worst", "in");
    bool passed = true;
    for (std::size_t index = 0; index < metrics.size(); ++index) {
        const Metric& metric = metrics[index];
        const Statistics& metricStatistics = statistics[index];
        const double meanAbsolute = metricStatistics.absoluteSum / conditionCount;
        const bool metricPasses = meanAbsolute <= metric.meanLimit;
        passed = passed && metricPasses;
        std::cout << std::format("{:<18}{:<5}{:>10.4f}{:>8.2f}{:>+10.4f}{:>+10.4f}  {:<8}{}\n", metric.name,
                                 metric.unit, meanAbsolute, metric.meanLimit,
                                 metricStatistics.sum / conditionCount, metricStatistics.worst,
                                 metricStatistics.worstCondition, metricPasses ? "" : "  FAILED");
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
