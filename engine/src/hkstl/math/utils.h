#ifndef HK_MATH_UTILS_H
#define HK_MATH_UTILS_H

#include <cmath>
#include "utility/hktypes.h"

namespace hkm {

template <typename T>
constexpr const T& clamp(const T &n, const T &lower, const T &upper)
{
    return (n <= lower ? lower : n <= upper ? n : upper);
}

template <typename T>
constexpr T sqrt(const T &val)
{
    return std::sqrt(val);
}

template <typename T>
constexpr i32 sign(const T &val)
{
    return (T(0) < val) - (val < T(0));
}

template<typename T>
constexpr const T& max(const T &a, const T &b)
{
    return (a > b) ? a : b;
}

template<typename T>
constexpr const T& min(const T &a, const T &b)
{
    return (a < b) ? a : b;
}

template <typename T>
constexpr T floor(const T &val) {
    // TODO: add check if val is a floating point value

    i64 i = static_cast<i64>(val);

    if (val < T(0) && (T)i != val) { i -= 1; }

    return static_cast<T>(i);
}

template<typename T>
constexpr T fract(const T &val)
{
    return val - floor(val);
}

b8 fuzzy_compare(f32 f1, f32 f2)
{
    // https://www.smallstepsystems.com/qt-compare-two-floats/
    constexpr f32 epsilon = 1.0e-05f;

    if (abs(f1 - f2) <= epsilon) { return true; }

    return abs(f1 - f2) <= epsilon * max(abs(f1), abs(f2));
}

} // hkm

#endif // HK_MATH_UTILS_H

