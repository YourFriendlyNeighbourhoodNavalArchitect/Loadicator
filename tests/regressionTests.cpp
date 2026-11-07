// Regression against the trim and stability book: every one of its 31 loading conditions is
// recomputed from data/conditions/condNN.csv and compared with the floating position printed in
// the book (data/conditions/expected.csv).
//
// Known, explained deviations get their own named tolerances instead of loosening every check:
//   - Partly filled holds of solid cargo: the cargo-hold tables (data/source/cargoHolds) have
//     12 rows and an implausible TCG column between 9 and 18 m sounding, so hold centroids differ
//     from the book by up to 0.08 m (LCG) and 0.26 m (TCG). Liquid tanks agree within 0.03 m.
//   - COND01 (lightship): at 2.7 m draught with 3.1 m trim the forefoot is almost out of the
//     water and KMT changes by ~25 m per metre of draught; interpolation misses it by ~1 %.

#include "loadicator/calc/analysis.h"
#include "loadicator/io/csv.h"
#include "loadicator/io/dataLoader.h"
#include "testing.h"

#include <cmath>
#include <filesystem>
#include <format>
#include <iostream>
#include <string>
#include <variant>

namespace {

using namespace loadicator;
namespace fs = std::filesystem;

const fs::path dataDirectory{LOADICATOR_TEST_DATA_DIR};

const Ship& ship() {
    static const Ship loaded = io::loadShip(dataDirectory);
    return loaded;
}

struct Tolerances {
    double displacement = 2.0;          // t   (book entries are rounded to 0.1 t each)
    double longitudinalCentre = 0.02;   // m   LCG
    double transverseCentre = 0.02;     // m   TCG
    double verticalCentre = 0.02;       // m   KG
    double draughtMidships = 0.025;     // m
    double draughtPerpendicular = 0.05; // m  TA, TF (includes the trim error)
    double trim = 0.06;                 // m
    double kmtRelative = 0.002;         // of KMT
    double heel = 0.1;                  // deg (book prints 0.1 deg)
};

/// Hold centroids for partly filled solid cargo cannot be reproduced from the hold tables.
constexpr Tolerances relaxForPartialSolidCargo(Tolerances tolerances) {
    tolerances.longitudinalCentre = 0.08;
    tolerances.transverseCentre = 0.25;
    tolerances.heel = 1.5;
    return tolerances;
}

[[nodiscard]] bool hasPartiallyFilledSolidCargo(const LoadingCondition& condition) {
    for (const TankLoad& load : condition.tankLoads) {
        if (load.phase != CargoPhase::solid) {
            continue;
        }
        const Volume capacity = ship().compartments.at(load.compartment).capacity();
        const Volume volume = std::holds_alternative<Mass>(load.amount)
                                  ? std::get<Mass>(load.amount) / load.density
                                  : std::get<Ratio>(load.amount) * capacity;
        if (volume.value() < 0.99 * capacity.value()) {
            return true;
        }
    }
    return false;
}

struct WorstDeviation {
    double value = 0.0;
    std::string condition;

    void update(double actual, double expected, const std::string& name) {
        if (std::abs(actual - expected) > value) {
            value = std::abs(actual - expected);
            condition = name;
        }
    }
};

void compareWithBook(EquilibriumMethod method, const Tolerances& baseTolerances) {
    const io::CsvDocument expected = io::CsvDocument::readFile(dataDirectory / "conditions" / "expected.csv");
    CHECK(expected.records().size() == 31);

    WorstDeviation draught, trim, heel, kmt;
    for (const io::CsvRecord& book : expected.records()) {
        const std::string name{book.text("condition")};
        const LoadingCondition condition =
            io::loadLoadingCondition(dataDirectory / "conditions" / (name + ".csv"));
        const Analysis result = analyzeLoadingCondition(ship(), condition, method);
        const FloatingPosition& position = result.floatingPosition;
        const InitialStability& stability = result.stability;

        Tolerances tolerances = hasPartiallyFilledSolidCargo(condition)
                                    ? relaxForPartialSolidCargo(baseTolerances)
                                    : baseTolerances;
        if (name == "cond01") {
            tolerances.kmtRelative = 0.02;
        }

        CHECK_NEAR(result.mass.displacement.value(), book.number("displacement"), tolerances.displacement);
        CHECK_NEAR(result.mass.centreOfGravity.x.value(), book.number("lcg"), tolerances.longitudinalCentre);
        CHECK_NEAR(result.mass.centreOfGravity.y.value(), book.number("tcg"), tolerances.transverseCentre);
        CHECK_NEAR(stability.kg.value(), book.number("kg"), tolerances.verticalCentre);
        CHECK_NEAR(position.draughtMidships.value(), book.number("draught"), tolerances.draughtMidships);
        CHECK_NEAR(position.draughtAft.value(), book.number("draughtAft"), tolerances.draughtPerpendicular);
        CHECK_NEAR(position.draughtForward.value(), book.number("draughtForward"),
                   tolerances.draughtPerpendicular);
        CHECK_NEAR(position.trim.value(), book.number("trim"), tolerances.trim);
        CHECK_NEAR(stability.kmt.value(), book.number("kmt"), tolerances.kmtRelative * book.number("kmt"));
        CHECK(!position.trimExtrapolated);
        CHECK(stability.heel.has_value());
        if (stability.heel) {
            CHECK_NEAR(stability.heel->degrees(), book.number("heel"), tolerances.heel);
            heel.update(stability.heel->degrees(), book.number("heel"), name);
        }

        draught.update(position.draughtMidships.value(), book.number("draught"), name);
        trim.update(position.trim.value(), book.number("trim"), name);
        kmt.update(stability.kmt.value(), book.number("kmt"), name);
    }

    std::cout
        << std::format("    worst deviation from the book, {}:\n", toString(method))
        << std::format(
               "      draught {:.3f} m ({}), trim {:.3f} m ({}), KMT {:.3f} m ({}), heel {:.2f} deg ({})\n",
               draught.value, draught.condition, trim.value, trim.condition, kmt.value, kmt.condition,
               heel.value, heel.condition);
}

TEST_CASE("regression/freeTrimMatchesBook") {
    compareWithBook(EquilibriumMethod::freeTrim, Tolerances{});
}

TEST_CASE("regression/firstOrderMatchesBook") {
    // Even-keel KMT ignores the effect of trim (up to 0.3 m here); the free-trim method does not.
    Tolerances tolerances;
    tolerances.trim = 0.07;
    tolerances.kmtRelative = 0.015;
    compareWithBook(EquilibriumMethod::firstOrder, tolerances);
}

TEST_CASE("regression/allConditionsBalanceToBookTotals") {
    // Each condition file must reproduce the book's total weight: catches extraction slips.
    const io::CsvDocument expected = io::CsvDocument::readFile(dataDirectory / "conditions" / "expected.csv");
    for (const io::CsvRecord& book : expected.records()) {
        const std::string name{book.text("condition")};
        const auto condition = io::loadLoadingCondition(dataDirectory / "conditions" / (name + ".csv"));
        double total = 0.0;
        for (const TankLoad& load : condition.tankLoads) {
            total += std::get<Mass>(load.amount).value();
        }
        for (const WeightItem& weight : condition.fixedWeights) {
            total += weight.mass.value();
        }
        CHECK_NEAR(total, book.number("displacement"), 2.0);
    }
}

} // namespace
