#ifndef HK_BITSET_H
#define HK_BITSET_H

#include "defines.h"

#include "hkbit.h"

#include <memory>
#include <string>
#include <type_traits>

namespace hk {

template <u64 N>
class bitset {
private:
    using T =
        std::conditional_t<N <= 8, u8,
            std::conditional_t<N <= 16, u16,
                std::conditional_t<N <= 32, u32, u64>
            >
        >;

public:
    constexpr bitset() : buffer_{} {};
    constexpr bitset(T val) : buffer_{val}
    {
        buffer_[elems() - 1] &= mask();
    }
    constexpr bitset(const std::string_view &str, u8 one = '1') : buffer_{}
    {
        for (u64 i = 0; i < str.size() && i < N; ++i) {
            if (str[i] == one) {
                set(N - 1 - i); // Set bits from right to left
            }
        }
    }
    constexpr bitset(const bitset &other) { *this = other; }

    constexpr bitset& operator=(const bitset &other)
    {
        std::memcpy(buffer_, other.buffer_, sizeof(T) * elems());
        return *this;
    }

    constexpr b8 operator==(const bitset &other) const
    {
        for (u64 i = 0; i < elems(); ++i) {
            if (buffer_[i] != other.buffer_[i]) { return false; }
        }
        return true;
    }

    /* ===== Element access ===== */

    // Accesses specific bit
    constexpr b8 operator[](u64 index) const
    {
        u64 num = index / (sizeof(T) * 8);
        u64 bit = index % (sizeof(T) * 8);
        return buffer_[num] >> bit & 0x01;
    }

    // Accesses specific bit
    constexpr b8 test(u64 index) const
    {
        ALWAYS_ASSERT(index < N, "Out of bounds");

        return (*this)[index];
    }

    // Checks if all of the bits are set to true
    constexpr b8 all() const
    {
        for (u64 i = 0; i < elems() - 1; ++i) {
            if (buffer_[i] != ~T{0}) { return false; }
        }
        return (buffer_[elems() - 1] & mask()) == mask();
    }

    // Checks if any of the bits are set to true
    constexpr b8 any() const
    {
        for (u64 i = 0; i < elems() - 1; ++i) {
            if (buffer_[i] != 0) { return true; }
        }
        return (buffer_[elems() - 1] & mask()) != 0;
    }

    // Checks if none of the bits are set to true
    constexpr b8 none() const
    {
        for (u64 i = 0; i < elems(); ++i) {
            if (buffer_[i] != 0) { return false; }
        }
        return (buffer_[elems() - 1] & mask()) == 0;
    }

    // Returns the number of bits set to true
    constexpr u64 count() const
    {
        u64 total = 0;
        for (u64 i = 0; i < elems() - 1; ++i) {
            total += hk::popcount(buffer_[i]);
        }
        total += hk::popcount(buffer_[elems() - 1] & mask());
        return total;
    }

    /* ===== Capacity ===== */

    // Returns the number of bits that the bitset holds
    constexpr T size() const { return N; }

    /* ===== Modifiers ===== */

    // Performs binary AND
    constexpr bitset& operator&=(const bitset &other)
    {
        for (u64 i = 0; i < elems(); ++i) {
            buffer_[i] &= other.buffer_[i];
        }
        return *this;
    }

    // Performs binary OR
    constexpr bitset& operator|=(const bitset &other)
    {
        for (u64 i = 0; i < elems(); ++i) {
            buffer_[i] |= other.buffer_[i];
        }
        return *this;
    }

    // Performs binary XOR
    constexpr bitset& operator^=(const bitset &other)
    {
        for (u64 i = 0; i < elems(); ++i) {
            buffer_[i] ^= other.buffer_[i];
        }
        return *this;
    }

    // Performs binary NOT
    constexpr bitset operator~() const
    {
        return bitset(*this).flip();
    }

    // Sets all bits to true
    constexpr bitset& set()
    {
        std::memset(buffer_, ~0, sizeof(T) * elems());

        // Clear the high bits beyond N
        buffer_[elems() - 1] &= mask();

        return *this;
    }

    // Sets the bit to true or given value
    constexpr bitset& set(u64 index, b8 value = true)
    {
        ALWAYS_ASSERT(index < N, "Out of bounds");

        if (value) {
            u64 num = index / (sizeof(T) * 8);
            u64 bit = index % (sizeof(T) * 8);
            buffer_[num] |= 1ULL << bit;
        } else {
            reset(index);
        }

        return *this;
    }

    // Sets all bits to false
    constexpr bitset& reset()
    {
        std::memset(buffer_, 0, sizeof(T) * elems());

        return *this;
    }

    // Sets the bit to false
    constexpr bitset& reset(u64 index) {
        ALWAYS_ASSERT(index < N, "Out of bounds");

        u64 num = index / (sizeof(T) * 8);
        u64 bit = index % (sizeof(T) * 8);
        buffer_[num] &= ~(1ULL << bit);

        return *this;
    }

    // Flips all bits
    constexpr bitset& flip()
    {
        for (u64 i = 0; i < elems(); ++i) {
            buffer_[i] = ~buffer_[i] & mask();
        }

        return *this;
    }

    // Flips the bit at specified position
    constexpr bitset& flip(u64 index) {
        ALWAYS_ASSERT(index < N, "Out of bounds");

        u64 num = index / (sizeof(T) * 8);
        u64 bit = index % (sizeof(T) * 8);
        buffer_[num] ^= 1ULL << bit;

        return *this;
    }

    /* ===== Conversions ===== */

    // Returns the contents of the bitset as a string
    inline std::string to_string(u8 zero = '0', u8 one = '1') const
    {
        std::string result(N, zero);

        for (u64 i = 0; i < N; ++i) {
            if (test(i)) {
                result[N - 1 - i] = one;
            }
        }

        return result;
    }

    // Returns the contents of the bitset as a underlying type
    constexpr T to_underlying() const
    {
        T result = 0;
        u64 current_bit = 0;

        for (u64 i = 0; i < elems(); ++i) {
            if (current_bit >= N) break;

            T elem = buffer_[i];
            if (i == elems() - 1) {
                elem &= mask();
            }

            result |= elem << current_bit;
            current_bit += sizeof(T) * 8;
        }

        return result;
    }

private:
    // Masks last bits
    constexpr u64 mask() const
    {
        return ~(~1ULL << ((N - 1) % (64)));
    }

    // Number of array indicies
    constexpr u64 elems() const
    {
        return (N + sizeof(T) * 8 - 1) / (sizeof(T) * 8);
    }

private:
    T buffer_[(N + sizeof(T) * 8 - 1) / (sizeof(T) * 8)];
};

template <u64 N>
constexpr bitset<N> operator&(const bitset<N> &lhs, const bitset<N> &rhs)
{
    bitset<N> out = lhs;
    out &= rhs;
    return out;
}

template <u64 N>
constexpr bitset<N> operator|(const bitset<N> &lhs, const bitset<N> &rhs)
{
    bitset<N> out = lhs;
    out |= rhs;
    return out;
}

template <u64 N>
constexpr bitset<N> operator^(const bitset<N> &lhs, const bitset<N> &rhs)
{
    bitset<N> out = lhs;
    out ^= rhs;
    return out;
}

}

#endif // HK_BITSET_H
