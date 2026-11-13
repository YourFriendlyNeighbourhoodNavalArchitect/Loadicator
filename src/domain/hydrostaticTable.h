#pragma once

#include "numeric/interpolation.h"
#include "util/quantity.h"

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
    // The trim grid holds every draught at every trim exactly once, in any order. The even-keel table covers
    // its draughts.
    HydrostaticTable(const std::vector<EvenKeelHydrostatics>& evenKeel, std::vector<TrimGridPoint> trimGrid);

    [[nodiscard]] Mass displacementAt(Length draught, Length trim) const;
    [[nodiscard]] Length lcbAt(Length draught, Length trim) const;
    [[nodiscard]] Length kmtAt(Length draught, Length trim) const;

    [[nodiscard]] Length lcfAt(Length draught) const;
    [[nodiscard]] Length vcbAt(Length draught) const;

    [[nodiscard]] Length minimumDraught() const { return Length{gridDraughts.front()}; }
    [[nodiscard]] Length maximumDraught() const { return Length{gridDraughts.back()}; }
    // The trims at which the hydrostatics are tabulated, in metres.
    [[nodiscard]] std::span<const double> tabulatedTrims() const { return gridTrims.nodes(); }

private:
    // The trim grid ordered by trim and then by draught, as the spline surfaces take it, and its axes.
    struct SortedGrid {
        std::vector<TrimGridPoint> points;
        numeric::Abscissa draughts;
        numeric::Abscissa trims;
    };

    [[nodiscard]] static SortedGrid sorted(std::vector<TrimGridPoint> trimGrid);

    HydrostaticTable(const std::vector<EvenKeelHydrostatics>& evenKeel, const SortedGrid& grid);

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
