#ifndef HK_BIT_H
#define HK_BIT_H

#include "defines.h"
#include "platform/info.h"

namespace hk {

// No way I'm using C++20 for this
#pragma warning(disable : 4068)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winvalid-constexpr"
// Detects whether the function call occurs within a constant-evaluated context
constexpr b8 is_constant_evaluated() noexcept {
#if defined(_MSC_VER)
    // https://stackoverflow.com/a/76271410
    struct C {};
    struct M : C { int a; };
    struct N : C { int a; };
    return &M::a != static_cast<int C::*>(&N::a);
#elif defined(__GNUC__)
    return __builtin_constant_p(42);
#else
    return false;
#endif
}
#pragma clang diagnostic pop
#pragma warning(default : 4068)

// Return the Hamming weight (The number of 1 bits)
// Accept integers of bit-widths upto 128
template <typename T = u32>
constexpr u32 popcount(T v)
{
    if (is_constant_evaluated()) {
        u32 count = 0;
        for (u32 i = 0; i < sizeof(T) * sizeof(u8); ++i) {
            if (v & (static_cast<T>(1) << i)) {
                count++;
            }
        }
        return count;
    }

    if (hk::platform::getCpuInfo().feature.popcnt) {
        return static_cast<u32>(__popcnt64(v));
    }

    // FIX: dosn't work for T = u64

    // SOURCE:
    // https://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetParallel
    v = v - ((v >> 1) & (T)~(T)0 / 3);
    v = (v & (T)~(T)0 / 15 * 3) + ((v >> 2) & (T)~(T)0 / 15 * 3);
    v = (v + (v >> 4)) & (T)~(T)0 / 255 * 15;
    u32 c = (T)(v * ((T)~(T)0 / 255)) >> (sizeof(T) - 1) * sizeof(u8);
    return c;
}

}

#endif // HK_BIT_H
