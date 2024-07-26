#ifndef HK_MATH_UTILS_H
#define HK_MATH_UTILS_H

namespace hkm {

template <typename T>
T clamp(const T &n, const T &lower, const T &upper)
{
    return (n <= lower ? lower : n <= upper ? n : upper);
}

}

#endif // HK_MATH_UTILS_H

