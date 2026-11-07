#pragma once

#include "loadicator/numeric/interpolation.h"
#include "loadicator/numeric/spline.h"
#include "loadicator/units/quantity.h"

#include <vector>

namespace loadicator {

/// Upright, even-keel hydrostatics at one draught (sea water, 1.025 t/m³).
struct EvenKeelHydrostatics {
    Length draught;
    Mass displacement;
    Length lcf;
    Length lcb;
    Length vcb;
    Length kmt;
    Moment mctOneCm; ///< moment to change trim by one centimetre
};

/// Upright hydrostatics at a given draught (at the reference point) and trim.
struct TrimmedHydrostatics {
    Mass displacement;
    Length lcb;
    Length kmt;
};

struct TrimGridPoint {
    Length draught;
    Length trim; ///< positive by the stern
    TrimmedHydrostatics values;
};

/// Hydrostatic tables of the ship: an even-keel table and a draught × trim grid.
class HydrostaticTable {
public:
    /// Throws unless draughts and displacements strictly increase and the grid is complete.
    HydrostaticTable(const std::vector<EvenKeelHydrostatics>& evenKeel,
                     const std::vector<TrimGridPoint>& trimGrid);

    /// Even-keel row interpolated at the given displacement.
    [[nodiscard]] EvenKeelHydrostatics evenKeelAt(Mass displacement) const;
    /// Even-keel row interpolated at the given draught.
    [[nodiscard]] EvenKeelHydrostatics evenKeelAtDraught(Length draught) const;

    /// Interpolated on the grid with natural cubic splines, first along draught, then along trim.
    /// Trims beyond the grid are extrapolated linearly; draughts beyond it are rejected.
    [[nodiscard]] TrimmedHydrostatics trimmedAt(Length draught, Length trim) const;

    [[nodiscard]] Length minimumDraught() const { return Length{gridDraughts.front()}; }
    [[nodiscard]] Length maximumDraught() const { return Length{gridDraughts.back()}; }
    [[nodiscard]] Length minimumTrim() const { return Length{gridTrims.front()}; }
    [[nodiscard]] Length maximumTrim() const { return Length{gridTrims.back()}; }

private:
    struct EvenKeelColumns {
        std::vector<double> draught;
        std::vector<double> displacement;
        std::vector<double> lcf;
        std::vector<double> lcb;
        std::vector<double> vcb;
        std::vector<double> kmt;
        std::vector<double> mctOneCm;
    };

    [[nodiscard]] EvenKeelHydrostatics evenKeelRow(numeric::Bracket bracket) const;

    EvenKeelColumns evenKeelColumns;
    numeric::Abscissa evenKeelDraughts;
    numeric::Abscissa evenKeelDisplacements;

    /// One spline along draught per tabulated trim.
    struct TrimColumn {
        numeric::NaturalCubicSpline displacement;
        numeric::NaturalCubicSpline lcb;
        numeric::NaturalCubicSpline kmt;
    };

    numeric::Abscissa gridDraughts;
    numeric::Abscissa gridTrims;
    std::vector<TrimColumn> trimColumns;
};

} // namespace loadicator
