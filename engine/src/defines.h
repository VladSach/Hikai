#ifndef HK_DEFINES_H
#define HK_DEFINES_H

/***************************
 * TYPES
 ***************************/

typedef bool b8;

typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long long u64;

typedef signed char      i8;
typedef signed short     i16;
typedef signed int       i32;
typedef signed long long i64;

typedef float  f32;
typedef double f64;

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
 * DEBUG
 ***************************/
#include "utils/Logger.h"

#include <cstdlib>

#define HKBREAK __debugbreak()

#define ALWAYS_ASSERT(expression, ...) \
	if (!(expression)) \
	{ \
		LOG_FATAL("Assertion failed:", __VA_ARGS__); \
		HKBREAK; \
		std::abort(); \
	}

#ifndef HKDEBUG
#define DEV_ASSERT(...)
#else
#define DEV_ASSERT(expression, ...) ALWAYS_ASSERT(expression, __VA_ARGS__);
#endif

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
