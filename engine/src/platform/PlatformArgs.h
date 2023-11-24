#ifndef HK_PLATFORMARGS_H
#define HK_PLATFORMARGS_H

#include "defines.h"

#ifdef _WIN32
#include "Windows/win.h"
#endif

class HKAPI PlatformArgs {
protected:
	PlatformArgs() {}
	static PlatformArgs *singleton;

public:
	PlatformArgs(PlatformArgs &other) = delete;
	void operator=(const PlatformArgs&) = delete;
	static PlatformArgs *instance();

public:
#ifdef _WIN32
    HINSTANCE hInstance;
    HINSTANCE hPrevInstance;
    LPSTR lpCmdLine;
    int nShowCmd;

    // PlatformArgs(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    //              LPSTR lpCmdLine, int nShowCmd)
    //     : hInstance(hInstance), hPrevInstance(hPrevInstance),
    //       lpCmdLine(lpCmdLine), nShowCmd(nShowCmd)
    // {}
#endif
};

#endif // HK_PLATFORMARGS_H
