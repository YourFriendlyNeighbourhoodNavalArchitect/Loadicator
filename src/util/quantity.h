#pragma once

#include <compare>
#include <functional>
#include <vector>

namespace loadicator {

// A quantity of dimension mass^MassExponent * length^LengthExponent, in tonnes and metres.
template <int MassExponent, int LengthExponent>
class Quantity {
public:
    constexpr Quantity() = default;
    constexpr explicit Quantity(double initialMagnitude) : magnitude(initialMagnitude) {}

    [[nodiscard]] constexpr double value() const { return magnitude; }

    constexpr Quantity& operator+=(Quantity other) {
        magnitude += other.magnitude;
        return *this;
    }
    constexpr Quantity& operator-=(Quantity other) {
        magnitude -= other.magnitude;
        return *this;
    }
    constexpr Quantity& operator*=(double factor) {
        magnitude *= factor;
        return *this;
    }
    constexpr Quantity& operator/=(double divisor) {
        magnitude /= divisor;
        return *this;
    }

    friend constexpr Quantity operator+(Quantity lhs, Quantity rhs) { return lhs += rhs; }
    friend constexpr Quantity operator-(Quantity lhs, Quantity rhs) { return lhs -= rhs; }
    friend constexpr Quantity operator*(Quantity lhs, double factor) { return lhs *= factor; }
    friend constexpr Quantity operator*(double factor, Quantity rhs) { return rhs *= factor; }
    friend constexpr Quantity operator/(Quantity lhs, double divisor) { return lhs /= divisor; }

    friend constexpr auto operator<=>(Quantity, Quantity) = default;

private:
    double magnitude = 0.0;
};

template <int MassA, int LengthA, int MassB, int LengthB>
[[nodiscard]] constexpr Quantity<MassA + MassB, LengthA + LengthB> operator*(Quantity<MassA, LengthA> lhs,
                                                                             Quantity<MassB, LengthB> rhs) {
    return Quantity<MassA + MassB, LengthA + LengthB>{lhs.value() * rhs.value()};
}

template <int MassA, int LengthA, int MassB, int LengthB>
[[nodiscard]] constexpr Quantity<MassA - MassB, LengthA - LengthB> operator/(Quantity<MassA, LengthA> lhs,
                                                                             Quantity<MassB, LengthB> rhs) {
    return Quantity<MassA - MassB, LengthA - LengthB>{lhs.value() / rhs.value()};
}

using Ratio = Quantity<0, 0>;
using Length = Quantity<0, 1>;
using Volume = Quantity<0, 3>;
using SecondMomentOfArea = Quantity<0, 4>;
using Mass = Quantity<1, 0>;
using Density = Quantity<1, -3>;
using Moment = Quantity<1, 1>;

// x forward of the aft perpendicular, y positive to port, z above the baseline.
struct Position {
    Length x;
    Length y;
    Length z;
};

// The magnitudes of a quantity projected from each row, for the numeric code, which works on plain numbers.
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
