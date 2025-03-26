#ifndef HK_TYPES_H
#define HK_TYPES_H

using b8 = bool;

using u8  = unsigned char;
using u16 = unsigned short;
using u32 = unsigned int;
using u64 = unsigned long long;

using i8  = signed char;
using i16 = signed short;
using i32 = signed int;
using i64 = signed long long;

using f32 = float;
using f64 = double;

#define STATIC_ASSERT static_assert

STATIC_ASSERT(sizeof(b8)  == 1, "Type error: b8 should be 1 byte");

STATIC_ASSERT(sizeof(u8)  == 1, "Type error: u8 should be 1 byte");
STATIC_ASSERT(sizeof(u16) == 2, "Type error: u16 should be 2 bytes");
STATIC_ASSERT(sizeof(u32) == 4, "Type error: u32 should be 4 bytes");
STATIC_ASSERT(sizeof(u64) == 8, "Type error: u64 should be 8 bytes");

STATIC_ASSERT(sizeof(i8)  == 1, "Type error: i8 should be 1 byte");
STATIC_ASSERT(sizeof(i16) == 2, "Type error: i16 should be 2 bytes");
STATIC_ASSERT(sizeof(i32) == 4, "Type error: i32 should be 4 bytes");
STATIC_ASSERT(sizeof(i64) == 8, "Type error: i64 should be 8 bytes");

STATIC_ASSERT(sizeof(f32) == 4, "Type error: i32 should be 4 bytes");
STATIC_ASSERT(sizeof(f64) == 8, "Type error: i64 should be 8 bytes");

// FIX: move this
// Placing types in namespace would defeat their purpose

namespace hk {

/* ===== Type Traits ===== */

template<typename T> struct remove_reference      { typedef T type; };
template<typename T> struct remove_reference<T&>  { typedef T type; };
template<typename T> struct remove_reference<T&&> { typedef T type; };
template<typename T>
using remove_reference_t = typename remove_reference<T>::type;

// Same SFINAE, but doesn't make my head spin like std's type deduction does
// template<typename T> constexpr b8 is_lvalue_reference(T)  { return false; }
// template<typename T> constexpr b8 is_lvalue_reference(T&) { return true; }

// Or to be more std-like
template<typename T>
class is_lvalue_reference {
    b8 value_ = false;
public:
    operator b8() const { return value_; }
};
template<typename T>
class is_lvalue_reference<T&> {
    b8 value_ = true;
public:
    operator b8() const { return value_; }
};
template<typename T>
constexpr b8 is_lvalue_reference_v = is_lvalue_reference<T>::value;

/* ===== Utils ===== */

// Indicate that an object MAY be moved
template <typename T>
constexpr remove_reference_t<T>&& move(T&& value)
{
    return static_cast<remove_reference_t<T>&&>(value);
}

// Forwards lvalues as either lvalues or as rvalues, depending on T
template <typename T>
constexpr T&& forward(remove_reference_t<T>& value)
{
    return static_cast<T&&>(value);
}

// Forwards rvalues as rvalues and prohibits forwarding of rvalues as lvalues
template <typename T>
constexpr T&& forward(remove_reference_t<T>&& value)
{
    // For cases like std::forward<int&>(7)
    STATIC_ASSERT(!is_lvalue_reference_v<T>);
    return static_cast<T&&>(value);
}

}

#endif // HK_TYPES_H
