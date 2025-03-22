#ifndef HK_RANDOM_H
#define HK_RANDOM_H

#include "utility/hktypes.h"

namespace hk {

// SOURCE:
// https://en.wikipedia.org/wiki/Xorshift#xoshiro256**
class xoshiro256ss {
public:
    xoshiro256ss(u64 seed = 1337)
    {
        // TODO: Probably shouldn't just copy the seed
        state[0] = seed;
        state[1] = seed;
        state[2] = seed;
        state[3] = seed;
    }

    u64 operator()()
    {
        u64 result = rol64(state[1] * 5, 7) * 9;
        u64 t = state[1] << 17;

        state[2] ^= state[0];
        state[3] ^= state[1];
        state[1] ^= state[2];
        state[0] ^= state[3];

        state[2] ^= t;
        state[3] = rol64(state[3], 45);

        return result;
    }

private:
    constexpr u64 rol64(u64 x, i32 k)
    {
        return (x << k) | (x >> (64 - k));
    }

private:
    u64 state[4];
};

}

#endif // HK_RANDOM_H
