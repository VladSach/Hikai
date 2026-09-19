#ifndef HK_ARGS_H
#define HK_ARGS_H

#include "platform/platform.h"

#ifdef HKWIN32
#include "platform/backend/Win32/win.h"
#endif

namespace hk::platform::args {

#if defined(HKWIN32)
    HINSTANCE hInstance;
    HINSTANCE hPrevInstance;
    LPSTR lpCmdLine;
    int nShowCmd;
#else
    int argc;
    char **argv = nullptr;
#endif

}

#endif // HK_ARGS_H
