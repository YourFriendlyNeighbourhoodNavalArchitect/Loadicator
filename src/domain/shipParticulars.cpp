#include "loadicator/domain/shipParticulars.h"

#include "loadicator/util/error.h"

#include <format>

namespace loadicator {

ShipParticulars::ShipParticulars(Length lengthBetweenPerpendiculars, Length referencePointX)
    : length(lengthBetweenPerpendiculars), referencePoint(referencePointX) {
    if (length <= Length{0.0}) {
        throw LoadicatorError(std::format("the length between perpendiculars of {} m is not positive",
                                          length.value()));
    }
    if (referencePoint < Length{0.0} || referencePoint > length) {
        throw LoadicatorError(
            std::format("the reference point at {} m lies outside the perpendiculars (0 .. {} m)",
                        referencePoint.value(), length.value()));
    }
}

}
