#pragma once

#include <cmath>
#include <compare>
#include <numbers>

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
    friend constexpr Quantity operator-(Quantity operand) { return Quantity{-operand.magnitude}; }
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

[[nodiscard]] constexpr Ratio ratioFromPercent(double percent) {
    return Ratio{percent / 100.0};
}

class Angle {
public:
    [[nodiscard]] static constexpr Angle fromRadians(double radians) { return Angle{radians}; }

    [[nodiscard]] constexpr double degrees() const { return radiansValue * 180.0 / std::numbers::pi; }

    friend constexpr auto operator<=>(Angle, Angle) = default;

private:
    constexpr explicit Angle(double radians) : radiansValue(radians) {}
    double radiansValue = 0.0;
};

[[nodiscard]] inline Angle arcTangent(Ratio ratio) {
    return Angle::fromRadians(std::atan(ratio.value()));
}

}
