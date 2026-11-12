#include "loadicator/calc/stability.h"

namespace loadicator {

InitialStability evaluateInitialStability(const MassSummary& mass, Length kmt) {
    const Length gmSolid = kmt - mass.centreOfGravity.z;
    const Length freeSurfaceCorrection = mass.freeSurfaceMoment / mass.displacement;
    const Length gmFluid = gmSolid - freeSurfaceCorrection;

    std::optional<Angle> heel;
    if (gmFluid > Length{0.0}) {
        heel = arcTangent(mass.centreOfGravity.y / gmFluid);
    }
    return InitialStability{
        .gmSolid = gmSolid, .freeSurfaceCorrection = freeSurfaceCorrection, .gmFluid = gmFluid, .heel = heel};
}

}
