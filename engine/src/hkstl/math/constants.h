#ifndef HK_CONSTANTS_H
#define HK_CONSTANTS_H

#include "utility/hktypes.h"

namespace hkm {

constexpr f32 pi  = 3.14159265358979323846f;
constexpr f32 tau = 6.28318530717958647692f;
constexpr f32 phi = 1.61803398874989484820f; // Golden Ratio

constexpr f32 degree2rad = pi / 180.f;
constexpr f32 rad2degree = 180.f / pi;

}

#endif // HK_CONSTANTS_H
