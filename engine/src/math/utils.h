#ifndef HK_MATH_UTILS_H
#define HK_MATH_UTILS_H

#include "defines.h"

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

}

#endif // HK_MATH_UTILS_H

