#include "loadicator/book/bookResults.h"
#include "loadicator/calc/analysis.h"
#include "loadicator/io/csv.h"
#include "loadicator/io/dataLoader.h"
#include "loadicator/io/schema.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <exception>
#include <filesystem>
#include <format>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace {

using namespace loadicator;
namespace schema = io::schema;

// The value of a metric, empty where the program does not compute it, as the heel of an unstable condition.
using MetricValue = std::optional<double>;

struct Metric {
    std::string_view name;
    std::string_view unit;
    std::string_view bookColumn;
    // The test fails when the mean absolute deviation over the conditions of the book exceeds this.
    double meanLimit;
    // A deviation beyond this is listed as an outlier, for information only.
    double outlierLimit;
    MetricValue (*computed)(const Analysis&);
};

constexpr std::array metrics{
    Metric{"Displacement", "t", book::column::displacement, 0.5, 1.5,
           [](const Analysis& a) -> MetricValue { return a.mass.displacement.value(); }},
    Metric{"LCG", "m", book::column::lcg, 0.02, 0.10,
           [](const Analysis& a) -> MetricValue { return a.mass.centreOfGravity.x.value(); }},
    Metric{"TCG", "m", book::column::tcg, 0.05, 0.10,
           [](const Analysis& a) -> MetricValue { return a.mass.centreOfGravity.y.value(); }},
    Metric{"KG", "m", book::column::kg, 0.01, 0.02,
           [](const Analysis& a) -> MetricValue { return a.mass.centreOfGravity.z.value(); }},
    Metric{"Draught midships", "m", book::column::draught, 0.02, 0.05,
           [](const Analysis& a) -> MetricValue { return a.floatingPosition.draughtMidships.value(); }},
    Metric{"Draught aft", "m", book::column::draughtAft, 0.02, 0.05,
           [](const Analysis& a) -> MetricValue { return a.floatingPosition.draughtAft.value(); }},
    Metric{"Draught forward", "m", book::column::draughtForward, 0.04, 0.05,
           [](const Analysis& a) -> MetricValue { return a.floatingPosition.draughtForward.value(); }},
    Metric{"Trim", "m", book::column::trim, 0.05, 0.10,
           [](const Analysis& a) -> MetricValue { return a.floatingPosition.trim.value(); }},
    Metric{"KMT", "m", book::column::kmt, 0.05, 0.10,
           [](const Analysis& a) -> MetricValue { return a.floatingPosition.kmt.value(); }},
    Metric{"GM solid", "m", book::column::gmSolid, 0.05, 0.10,
           [](const Analysis& a) -> MetricValue { return a.stability.gmSolid.value(); }},
    Metric{"GM fluid", "m", book::column::gm, 0.05, 0.10,
           [](const Analysis& a) -> MetricValue { return a.stability.gmFluid.value(); }},
    Metric{"Heel", "deg", book::column::heel, 0.4, 0.5,
           [](const Analysis& a) -> MetricValue {
               return a.stability.heel ? MetricValue{a.stability.heel->degrees()} : std::nullopt;
           }},
};

struct Outlier {
    std::string condition;
    double computed = 0.0;
    double book = 0.0;
};

struct Statistics {
    std::size_t count = 0;
    double sum = 0.0;
    double absoluteSum = 0.0;
    double worst = 0.0;
    std::string worstCondition;
    std::vector<Outlier> outliers;
    std::vector<std::string> notComputed;

    void add(const std::string& condition, MetricValue computed, double book, double outlierLimit) {
        if (!computed) {
            notComputed.push_back(condition);
            return;
        }
        const double deviation = *computed - book;
        ++count;
        sum += deviation;
        absoluteSum += std::abs(deviation);
        if (std::abs(deviation) >= std::abs(worst)) {
            worst = deviation;
            worstCondition = condition;
        }
        if (std::abs(deviation) > outlierLimit) {
            outliers.push_back(Outlier{condition, *computed, book});
        }
    }

    [[nodiscard]] double mean() const { return sum / static_cast<double>(count); }
    [[nodiscard]] double meanAbsolute() const { return absoluteSum / static_cast<double>(count); }

    // The mean covers every condition of the book only when the metric was computed for all of them.
    [[nodiscard]] bool passes(double meanLimit) const {
        return notComputed.empty() && count > 0 && meanAbsolute() <= meanLimit;
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
        const Analysis analysis = analyzeLoadingCondition(
            ship, io::loadLoadingCondition(dataDirectory / schema::files::conditions /
                                           (name + std::string(schema::files::extension))));
        for (std::size_t index = 0; index < metrics.size(); ++index) {
            const Metric& metric = metrics[index];
            statistics[index].add(name, metric.computed(analysis), expected.number(columns[index]),
                                  metric.outlierLimit);
        }
    }

    std::cout << std::format("Deviation from the trim and stability book over {} loading conditions\n",
                             bookResults.rowCount())
              << "(computed minus book; the test fails when a mean absolute deviation exceeds its limit)\n\n"
              << std::format("{:<18}{:<5}{:>10}{:>8}{:>10}{:>10}  {:<8}{:>9}\n", "Metric", "Unit",
                             "Mean abs", "Limit", "Mean", "Worst", "in", "Outliers");
    bool passed = true;
    for (std::size_t index = 0; index < metrics.size(); ++index) {
        const Metric& metric = metrics[index];
        const Statistics& metricStatistics = statistics[index];
        const bool metricPasses = metricStatistics.passes(metric.meanLimit);
        passed = passed && metricPasses;
        if (metricStatistics.count == 0) {
            std::cout << std::format("{:<18}{:<5}{:>10}{:>8.2f}  FAILED\n", metric.name, metric.unit, "-",
                                     metric.meanLimit);
            continue;
        }
        std::cout << std::format("{:<18}{:<5}{:>10.4f}{:>8.2f}{:>+10.4f}{:>+10.4f}  {:<8}{:>9}{}\n",
                                 metric.name, metric.unit, metricStatistics.meanAbsolute(), metric.meanLimit,
                                 metricStatistics.mean(), metricStatistics.worst,
                                 metricStatistics.worstCondition, metricStatistics.outliers.size(),
                                 metricPasses ? "" : "  FAILED");
        for (const std::string& name : metricStatistics.notComputed) {
            std::cout << std::format("  not computed for {}, so the mean does not cover every condition\n",
                                     name);
        }
    }

    std::cout << "\nOutliers, for information: deviations beyond the outlier limit of their metric\n\n"
              << std::format("{:<10}{:<18}{:>12}{:>12}{:>12}{:>8}\n", "Condition", "Metric", "Computed",
                             "Book", "Deviation", "Limit");
    for (std::size_t index = 0; index < metrics.size(); ++index) {
        for (const Outlier& outlier : statistics[index].outliers) {
            std::cout << std::format("{:<10}{:<18}{:>12.3f}{:>12.3f}{:>+12.3f}{:>8.2f}\n",
                                     outlier.condition, metrics[index].name, outlier.computed, outlier.book,
                                     outlier.computed - outlier.book, metrics[index].outlierLimit);
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
