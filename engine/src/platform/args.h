#ifndef HK_ARGS_H
#define HK_ARGS_H

#include "defines.h"
#include "platform.h"

#ifdef HKWINDOWS
#include "Windows/win.h"
#endif

namespace hk::platform::args {

#if defined(HKWINDOWS)
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
