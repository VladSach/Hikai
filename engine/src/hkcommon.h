#ifndef HK_COMMON_H
#define HK_COMMON_H

/* ===== DLL ===== */
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

#endif // HK_COMMON_H
