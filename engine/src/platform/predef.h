#ifndef HK_PREDEF_H
#define HK_PREDEF_H

// Platform detection
#if defined(_WIN32)
    #define HKWIN32
#elif defined(__linux__)
    #define HKLINUX
#else
    #error "Unsupported platform"
#endif

// Compiler detection
#if defined(_MSC_VER)
    #define HKMSVC
#elif defined(__GNUC__)
    #define HKGNUC
#elif defined(__clang__)
    #define HKCLANG
#elif defined(__MINGW32__) || defined(__MINGW64__)
    #define HKMINGW
#else
    #error "Unsupported compiler"
#endif

#endif // HK_PREDEF_H
