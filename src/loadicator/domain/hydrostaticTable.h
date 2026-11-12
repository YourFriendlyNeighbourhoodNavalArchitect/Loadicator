#pragma once

#include "loadicator/numeric/interpolation.h"
#include "loadicator/numeric/splineSurface.h"
#include "loadicator/util/quantity.h"

#include <span>
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
    // The trims at which the hydrostatics are tabulated, in metres.
    [[nodiscard]] std::span<const double> tabulatedTrims() const { return gridTrims.nodes(); }

private:
    // The axes of the trim grid and its points, ordered by trim and then by draught.
    struct TrimGrid {
        numeric::Abscissa draughts;
        numeric::Abscissa trims;
        std::vector<TrimGridPoint> points;
    };

    [[nodiscard]] static TrimGrid orderedGrid(const std::vector<TrimGridPoint>& trimGrid);

    HydrostaticTable(const std::vector<EvenKeelHydrostatics>& evenKeel, const TrimGrid& grid);

    [[nodiscard]] double valueAt(const numeric::SplineSurface& surface, Length draught, Length trim) const;

    numeric::Abscissa evenKeelDraughts;
    std::vector<double> lcf;
    std::vector<double> vcb;

    numeric::Abscissa gridDraughts;
    numeric::Abscissa gridTrims;
    numeric::SplineSurface displacement;
    numeric::SplineSurface lcb;
    numeric::SplineSurface kmt;
};

}
