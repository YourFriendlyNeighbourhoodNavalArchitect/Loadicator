#include "verification.h"

#include "loadicator/book/bookResults.h"
#include "loadicator/calc/loadResolution.h"
#include "loadicator/calc/massSummary.h"
#include "loadicator/util/error.h"

#include <cmath>

namespace loadicator::extractor {

namespace {

// The book prints masses to 0.1 t.
constexpr double massRounding = 0.05;

}

void verify(const ShipData& data) {
    (void)HydrostaticTable(data.hydrostatics.evenKeel, data.hydrostatics.trimGrid);

    CompartmentLibrary compartments;
    for (const CompartmentData& table : data.compartments) {
        compartments.add(CompartmentTable(CompartmentId{table.id}, table.rows));
    }

    for (const BookCondition& bookCondition : data.conditions) {
        const std::vector<WeightItem> weights =
            resolveLoadingCondition(bookCondition.condition, compartments);
        const double total = summarizeWeights(weights).displacement.value();
        const double bookTotal = bookCondition.results.at(book::column::displacement);
        if (std::abs(total - bookTotal) > massRounding * static_cast<double>(weights.size() + 1)) {
            throw LoadicatorError(bookCondition.condition.name() + ": the entries sum to " +
                                  std::to_string(total) + " t, but the book gives a total weight of " +
                                  std::to_string(bookTotal) + " t");
        }
    }
}

}
