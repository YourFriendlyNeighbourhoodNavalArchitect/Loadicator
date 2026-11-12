#pragma once

#include <functional>
#include <vector>

namespace loadicator {

// The magnitudes of a quantity projected from each row, for the numeric layer, which works on plain numbers.
template <typename Row, typename Projection>
[[nodiscard]] std::vector<double> magnitudes(const std::vector<Row>& rows, Projection projection) {
    std::vector<double> values;
    values.reserve(rows.size());
    for (const Row& row : rows) {
        values.push_back(std::invoke(projection, row).value());
    }
    return values;
}

}
