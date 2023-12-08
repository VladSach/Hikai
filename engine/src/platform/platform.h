#ifndef HK_PLATFORM_H
#define HK_PLATFORM_H

// TODO: use better macros for conditional compilation
#ifdef _WIN32
#include "Windows/WinLog.h"
#include "Windows/WinFile.h"
#include "Windows/WinWindow.h"

// This, alongside PlatformArgs, is a complitly garbage way to do this
// but at least it works so I let it be as it is till better days
#define PLATFORM_MAIN "platform/Windows/WinMain.h"
#endif // _WIN32


#endif // HK_PLATFORM_H
