#ifndef HK_PLATFORMARGS_H
#define HK_PLATFORMARGS_H

#include "defines.h"
#include "platform.h"

#ifdef HKWINDOWS
#include "Windows/win.h"
#endif

class HKAPI PlatformArgs {
protected:
    PlatformArgs() {}

public:
    PlatformArgs(PlatformArgs &other) = delete;
    void operator=(const PlatformArgs&) = delete;

    static PlatformArgs *instance() {
        static PlatformArgs singleton;
        return &singleton;
    }

public:
#if defined(HKWINDOWS)
    HINSTANCE hInstance;
    HINSTANCE hPrevInstance;
    LPSTR lpCmdLine;
    int nShowCmd;

    // PlatformArgs(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    //              LPSTR lpCmdLine, int nShowCmd)
    //     : hInstance(hInstance), hPrevInstance(hPrevInstance),
    //       lpCmdLine(lpCmdLine), nShowCmd(nShowCmd)
    // {}
#else
    int argc;
    char **argv = nullptr;
#endif
};

#endif // HK_PLATFORMARGS_H
