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
    #include "platform/Windows/win.h"
    #include "platform/Windows/Window.h"
    #include "platform/Windows/WinLog.h"

    // This, alongside platform::args, is a complitly garbage way to do this
    // but at least it works so I let it be as it is till better days
    #define PLATFORM_MAIN "platform/Windows/main.h"
#endif // Platform dependent includes

#endif // HK_PLATFORM_H
