#ifndef HK_BITFLAG_H
#define HK_BITFLAG_H

#include "hkbitset.h"
#include "utility/hktypes.h"

#include <type_traits>
#include <limits>

/* ===== Usage =====
 * enum class YourEnumFlags {
 *     First  = 1 << 1,
 *     Second = 1 << 2,
 *     Third  = 1 << 3,
 *     ...
 * };
 * REGISTER_ENUM(YourEnumFlags); // Somewhere in header
 *
 * hk::bitflag<YourEnumFlags> flags;
 *
 * Treat flags as bitset but with enum access
 */

namespace hk {

template <typename Enum>
struct is_valid_enum {
    static constexpr b8 value = false;
};

#define REGISTER_ENUM(EnumType) \
    template<> struct is_valid_enum<EnumType> { \
        static constexpr b8 value = true; \
    }

template<typename Enum>
class bitflag {
private:
    using T = std::underlying_type_t<Enum>;

public:
    constexpr bitflag() : flags_() {}
    constexpr bitflag(Enum flag) : flags_(static_cast<T>(flag)) {};
    constexpr bitflag(hk::bitset<sizeof(T) * 8> flags) : flags_(flags) {};

    constexpr bitflag(const bitflag &other) = default;
    constexpr bitflag& operator=(const bitflag &other) = default;

    // Cast operators
    explicit constexpr operator b8() const
    {
        return flags_.any();
    }

    // Relational operators
    constexpr b8 operator<(bitflag const &rhs) const
    {
        return flags_.to_underlying() < rhs.flags_.to_underlying();
    }
    constexpr b8 operator<=(bitflag const &rhs) const
    {
        return flags_.to_underlying() <= rhs.flags_.to_underlying();
    }
    constexpr b8 operator>(bitflag const &rhs) const
    {
        return flags_.to_underlying() > rhs.flags_.to_underlying();
    }
    constexpr b8 operator>=(bitflag const &rhs) const
    {
        return flags_.to_underlying() >= rhs.flags_.to_underlying();
    }
    constexpr b8 operator==(bitflag const &rhs) const
    {
        return flags_.to_underlying() == rhs.flags_.to_underlying();
    }
    constexpr b8 operator!=(bitflag const &rhs) const
    {
        return flags_.to_underlying() != rhs.flags_.to_underlying();
    }

    // Logical operators
    constexpr b8 operator!() const {
        return !flags_.any();
    }

    // Bitwise operators
    constexpr bitflag operator&(bitflag const &rhs) const
    {
        return bitflag(flags_ & rhs.flags_);
    }

    constexpr bitflag operator|(bitflag const &rhs) const
    {
        return bitflag(flags_ | rhs.flags_);
    }

    constexpr bitflag operator^(bitflag const &rhs) const
    {
        return bitflag(flags_ ^ rhs.flags_);
    }

    constexpr bitflag operator~() const {
        return bitflag(~flags_);
    }

    // Assignment operators
    constexpr bitflag& operator&=(bitflag const &rhs)
    {
        flags_ &= rhs.flags_;
        return *this;
    }

    constexpr bitflag& operator|=(bitflag const &rhs)
    {
        flags_ |= rhs.flags_;
        return *this;
    }

    constexpr bitflag& operator^=(bitflag const &rhs)
    {
        flags_ ^= rhs.flags_;
        return *this;
    }

    T value() const
    {
        return flags_.to_underlying();
    }

private:
    hk::bitset<sizeof(T) * 8> flags_;
};

// template <typename Enum>
// constexpr bitflag<Enum> operator&(Enum bit, bitflag<Enum> const &flags)
// {
//     return flags.operator&(bit);
// }
//
// template <typename Enum>
// constexpr bitflag<Enum> operator&(bitflag<Enum> const &flags, Enum bit)
// {
//     return flags.operator&(bit);
// }

// ===== Bitwise operators on Registered Enums =====

// PERF: can also wrap this in namespace
// I won't have to register enums, but then all enums be visible
// to use write `using namespace hk::enum_ops` for instance
// but there is no unuse namespace, so it's also not the best option

// Returns binary AND
template <typename Enum, std::enable_if_t<std::is_enum_v<Enum>, bool> = true>
constexpr hk::bitflag<Enum> operator&(Enum lhs, Enum rhs)
{
    return hk::bitflag<Enum>(lhs) & rhs;
}

// Returns binary OR
template <typename Enum,
          std::enable_if_t<hk::is_valid_enum<Enum>::value, bool> = true>
constexpr hk::bitflag<Enum> operator|(Enum lhs, Enum rhs)
{
    return hk::bitflag<Enum>(lhs) | rhs;
}

// Returns binary XOR
template <typename Enum,
          std::enable_if_t<hk::is_valid_enum<Enum>::value, bool> = true>
constexpr hk::bitflag<Enum> operator^(Enum lhs, Enum rhs)
{
    return hk::bitflag<Enum>(lhs) ^ rhs;
}

// Returns binary NOT
template <typename Enum,
          std::enable_if_t<hk::is_valid_enum<Enum>::value, bool> = true>
constexpr hk::bitflag<Enum> operator~(Enum lhs)
{
    return ~(hk::bitflag<Enum>(lhs));
}

}

#endif // HK_BITFLAG_H
