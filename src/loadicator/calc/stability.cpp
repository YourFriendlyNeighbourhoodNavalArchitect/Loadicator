#include "loadicator/calc/stability.h"

#include <cmath>

namespace loadicator {

InitialStability evaluateInitialStability(const MassSummary& mass, Length kmt) {
    const Length kg = mass.centreOfGravity.z;
    const Length gmSolid = kmt - kg;
    const Length freeSurfaceCorrection = mass.freeSurfaceMoment / mass.displacement;
    const Length gmFluid = gmSolid - freeSurfaceCorrection;

    std::optional<Angle> heel;
    if (gmFluid.value() > 0.0) {
        heel = Angle::fromRadians(std::atan((mass.centreOfGravity.y / gmFluid).value()));
    }
    return InitialStability{.kg = kg,
                            .kmt = kmt,
                            .gmSolid = gmSolid,
                            .freeSurfaceCorrection = freeSurfaceCorrection,
                            .gmFluid = gmFluid,
                            .heel = heel};
}

} // namespace loadicator
