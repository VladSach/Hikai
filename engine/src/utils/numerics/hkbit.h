#ifndef HK_BIT_H
#define HK_BIT_H

#include "defines.h"

namespace hk {

// Return the Hamming weight (The number of 1 bits)
// Accept integers of bit-widths upto 128
template <typename T = u32>
constexpr u32 popcount(T v)
{
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
