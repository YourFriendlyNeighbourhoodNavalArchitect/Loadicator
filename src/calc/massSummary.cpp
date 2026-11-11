#include "calc/massSummary.h"

#include "util/error.h"

namespace loadicator {

MassSummary summarizeWeights(std::span<const WeightItem> items) {
    Mass total{0.0};
    Moment longitudinal{0.0};
    Moment transverse{0.0};
    Moment vertical{0.0};
    Moment freeSurface{0.0};
    for (const WeightItem& item : items) {
        total += item.mass;
        longitudinal += item.mass * item.centre.x;
        transverse += item.mass * item.centre.y;
        vertical += item.mass * item.centre.z;
        freeSurface += item.freeSurfaceMoment;
    }
    if (total <= Mass{0.0}) {
        throw LoadicatorError("total displacement must be positive");
    }
    return MassSummary{.displacement = total,
                       .centreOfGravity =
                           Position{longitudinal / total, transverse / total, vertical / total},
                       .freeSurfaceMoment = freeSurface};
}

}
