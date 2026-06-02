#ifndef HK_WIN_H
#define HK_WIN_H

#ifdef UNDEFINED_MIN
#pragma pop_macro("min")
#endif

#ifdef UNDEFINED_MAX
#pragma pop_macro("max")
#endif

#ifdef UNDEFINED_NEAR
#pragma pop_macro("near")
#endif

#ifdef UNDEFINED_FAR
#pragma pop_macro("far")
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>

#ifdef min
#define UNDEFINED_MIN
#pragma push_macro("min")
#undef min
#endif

#ifdef max
#define UNDEFINED_MAX
#pragma push_macro("max")
#undef max
#endif

#ifdef near
#define UNDEFINED_NEAR
#pragma push_macro("near")
#undef near
#endif

#ifdef far
#define UNDEFINED_FAR
#pragma push_macro("far")
#undef far
#endif

#endif // HK_WIN_H
