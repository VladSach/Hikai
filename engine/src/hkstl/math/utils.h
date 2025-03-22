#ifndef HK_MATH_UTILS_H
#define HK_MATH_UTILS_H

#include <cmath>

namespace hkm {

template <typename T>
constexpr T clamp(const T &n, const T &lower, const T &upper)
{
    return (n <= lower ? lower : n <= upper ? n : upper);
}


template <typename T>
constexpr T sqrt(const T &num)
{
    return std::sqrt(num);
}

template<typename T>
const T& max(const T &a, const T &b)
{
    return (a > b) ? a : b;
}

template<typename T>
const T& min(const T &a, const T &b)
{
    return (a < b) ? a : b;
}

}

#endif // HK_MATH_UTILS_H

