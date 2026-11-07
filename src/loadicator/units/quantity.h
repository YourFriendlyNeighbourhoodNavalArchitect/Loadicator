#pragma once

#include <cmath>
#include <compare>
#include <numbers>

namespace loadicator {

/// A physical quantity of dimension mass^MassExponent * length^LengthExponent.
///
/// Base units are tonnes and metres, the units of every source table. The dimension is part
/// of the type, so adding a moment to a length, or passing a volume where a mass is expected,
/// does not compile. Multiplication and division derive the resulting dimension.
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

template <int MassExponent, int LengthExponent>
[[nodiscard]] Quantity<MassExponent, LengthExponent> abs(Quantity<MassExponent, LengthExponent> quantity) {
    return Quantity<MassExponent, LengthExponent>{std::abs(quantity.value())};
}

using Ratio = Quantity<0, 0>;              ///< dimensionless
using Length = Quantity<0, 1>;             ///< m
using Volume = Quantity<0, 3>;             ///< m³
using SecondMomentOfArea = Quantity<0, 4>; ///< m⁴ (free-surface inertia)
using Mass = Quantity<1, 0>;               ///< t
using Density = Quantity<1, -3>;           ///< t/m³
using Moment = Quantity<1, 1>;             ///< t·m (also free-surface moment)

[[nodiscard]] constexpr Ratio ratioFromPercent(double percent) {
    return Ratio{percent / 100.0};
}
[[nodiscard]] constexpr double toPercent(Ratio ratio) {
    return ratio.value() * 100.0;
}

/// Plane angle; kept separate from Ratio so radians and degrees cannot be confused.
class Angle {
public:
    [[nodiscard]] static constexpr Angle fromRadians(double radians) { return Angle{radians}; }
    [[nodiscard]] static constexpr Angle fromDegrees(double degrees) {
        return Angle{degrees * std::numbers::pi / 180.0};
    }

    [[nodiscard]] constexpr double radians() const { return radiansValue; }
    [[nodiscard]] constexpr double degrees() const { return radiansValue * 180.0 / std::numbers::pi; }

    friend constexpr auto operator<=>(Angle, Angle) = default;

private:
    constexpr explicit Angle(double radians) : radiansValue(radians) {}
    double radiansValue = 0.0;
};

namespace literals {

[[nodiscard]] constexpr Mass operator""_t(long double value) {
    return Mass{static_cast<double>(value)};
}
[[nodiscard]] constexpr Mass operator""_t(unsigned long long value) {
    return Mass{static_cast<double>(value)};
}
[[nodiscard]] constexpr Length operator""_m(long double value) {
    return Length{static_cast<double>(value)};
}
[[nodiscard]] constexpr Length operator""_m(unsigned long long value) {
    return Length{static_cast<double>(value)};
}
[[nodiscard]] constexpr Volume operator""_m3(long double value) {
    return Volume{static_cast<double>(value)};
}
[[nodiscard]] constexpr Volume operator""_m3(unsigned long long value) {
    return Volume{static_cast<double>(value)};
}
[[nodiscard]] constexpr Density operator""_tPerM3(long double value) {
    return Density{static_cast<double>(value)};
}

} // namespace literals

} // namespace loadicator
