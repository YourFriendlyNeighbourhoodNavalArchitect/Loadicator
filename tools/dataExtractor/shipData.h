#pragma once

#include "loadicator/domain/compartmentTable.h"
#include "loadicator/domain/hydrostaticTable.h"
#include "loadicator/domain/loadingCondition.h"
#include "loadicator/domain/shipParticulars.h"

#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace loadicator::extractor {

struct CompartmentData {
    std::string id;
    std::vector<CompartmentTable::Row> rows;
};

struct HydrostaticData {
    ShipParticulars particulars;
    std::vector<EvenKeelHydrostatics> evenKeel;
    std::vector<TrimGridPoint> trimGrid;
};

using BookResults = std::map<std::string_view, double>;

struct BookCondition {
    int number = 0;
    LoadingCondition condition;
    BookResults results;
};

struct ShipData {
    HydrostaticData hydrostatics;
    std::vector<CompartmentData> compartments;
    std::vector<BookCondition> conditions;
};

}
