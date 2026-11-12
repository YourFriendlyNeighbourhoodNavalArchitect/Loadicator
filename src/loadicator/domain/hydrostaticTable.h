#pragma once

#include "loadicator/numeric/interpolation.h"
#include "loadicator/numeric/spline.h"
#include "loadicator/util/quantity.h"

#include <vector>

namespace loadicator {

struct EvenKeelHydrostatics {
    Length draught;
    Length lcf;
    Length vcb;
};

// Draught at the reference point of the tables, trim positive by the stern, sea water of 1.025 t/m3.
struct TrimGridPoint {
    Length draught;
    Length trim;
    Mass displacement;
    Length lcb;
    Length kmt;
};

class HydrostaticTable {
public:
    HydrostaticTable(const std::vector<EvenKeelHydrostatics>& evenKeel,
                     const std::vector<TrimGridPoint>& trimGrid);

    [[nodiscard]] Mass displacementAt(Length draught, Length trim) const;
    [[nodiscard]] Length lcbAt(Length draught, Length trim) const;
    [[nodiscard]] Length kmtAt(Length draught, Length trim) const;

    [[nodiscard]] Length lcfAt(Length draught) const;
    [[nodiscard]] Length vcbAt(Length draught) const;

    [[nodiscard]] Length minimumDraught() const { return Length{gridDraughts.front()}; }
    [[nodiscard]] Length maximumDraught() const { return Length{gridDraughts.back()}; }
    [[nodiscard]] Length minimumTrim() const { return Length{gridTrims.front()}; }
    [[nodiscard]] Length maximumTrim() const { return Length{gridTrims.back()}; }

private:
    using Surface = std::vector<numeric::NaturalCubicSpline>;

    [[nodiscard]] double valueAt(const Surface& surface, Length draught, Length trim) const;

    numeric::Abscissa evenKeelDraughts;
    std::vector<double> lcf;
    std::vector<double> vcb;

    numeric::Abscissa gridDraughts;
    numeric::Abscissa gridTrims;
    // One spline over draught per tabulated trim.
    Surface displacement;
    Surface lcb;
    Surface kmt;
    // A natural cubic spline is linear in its ordinates, so the spline over trim through the values at one
    // draught is the sum of those values, each weighted by the spline that is one at its trim and zero at the
    // others. These weights depend on the trim alone, which spares a spline over trim for every lookup.
    std::vector<numeric::NaturalCubicSpline> trimBasis;
};

}
