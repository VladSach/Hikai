#ifndef HK_PLATFORM_H
#define HK_PLATFORM_H

#include "predef.h"

// Platform dependent main
#if defined(HKWIN32)
    #define PLATFORM_MAIN "platform/entry/win32_main.h"
#elif defined (HKLINUX)
    #define PLATFORM_MAIN "platform/entry/linux_main.h"
#endif

#include "platform/backend/backend.h"
#include "platform/console/console.h"
#include "platform/filesystem/filesystem.h"
#include "platform/specs/specs.h"
#include "platform/window/Window.h"

#endif // HK_PLATFORM_H
