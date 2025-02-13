#ifndef HK_DEFINES_H
#define HK_DEFINES_H

/***************************
 * TYPES
 ***************************/

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

/***************************
 * DLL
 ***************************/
#ifdef HKDLL_OUT
    #ifdef _MSC_VER
    #define HKAPI __declspec(dllexport)
    #else
    #define HKAPI
    #endif
#else
    #ifdef _MSC_VER
    #define HKAPI __declspec(dllimport)
    #else
    #define HKAPI
    #endif
#endif

#endif // HK_DEFINES_H
