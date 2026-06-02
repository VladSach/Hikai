#ifndef HK_PLATFORM_H
#define HK_PLATFORM_H

#include "predef.h"

// Platform dependent main
#if defined(HKWIN32)
    // This, alongside platform::args, is a complitly garbage way to do this
    // but at least it works so I let it be as it is till better days
    #define PLATFORM_MAIN "platform/entry/win_main.h"
#elif defined (HKLINUX)
    #define PLATFORM_MAIN "platform/entry/linux_main.h"
#endif

#include "platform/backend/backend.h"
#include "platform/console/console.h"
#include "platform/filesystem/filesystem.h"
#include "platform/specs/specs.h"
#include "platform/window/Window.h"

#endif // HK_PLATFORM_H
