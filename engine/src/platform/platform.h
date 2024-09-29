#ifndef HK_PLATFORM_H
#define HK_PLATFORM_H

// Platform detection
#if defined(_WIN32)
    #define HKWINDOWS
#else
    #error "Unsupported platform"
#endif

// Platform dependent includes
#if defined(HKWINDOWS)
    #include "Windows/WinLog.h"
    #include "Windows/WinWindow.h"

    // This, alongside PlatformArgs, is a complitly garbage way to do this
    // but at least it works so I let it be as it is till better days
    #define PLATFORM_MAIN "platform/Windows/WinMain.h"
#endif // Platform dependent includes

#include "filesystem.h"

#endif // HK_PLATFORM_H
