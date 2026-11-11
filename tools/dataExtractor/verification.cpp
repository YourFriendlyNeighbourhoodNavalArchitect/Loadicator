#include "verification.h"

#include "calc/loadResolution.h"
#include "calc/massSummary.h"
#include "io/schema.h"
#include "util/error.h"

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

    for (const BookCondition& book : data.conditions) {
        const std::vector<WeightItem> weights = resolveLoadingCondition(book.condition, compartments);
        const double total = summarizeWeights(weights).displacement.value();
        const double bookTotal = book.results.at(io::schema::bookResults::displacement);
        if (std::abs(total - bookTotal) > massRounding * static_cast<double>(weights.size() + 1)) {
            throw LoadicatorError(book.condition.name + ": the entries sum to " + std::to_string(total) +
                                  " t, but the book gives a total weight of " + std::to_string(bookTotal) +
                                  " t");
        }
    }
}

}
