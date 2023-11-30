#ifndef HK_WINMAIN_H
#define HK_WINMAIN_H

#include "core/Application.h"
#include "platform/platform.h"
#include "platform/PlatformArgs.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nShowCmd)
{
#ifdef HKDEBUG
    allocWinConsole();
    setLogFile("hikai_log.txt");
#endif

    LOG_INFO("Initializing Windows startup");

    PlatformArgs &args = *PlatformArgs::instance();
    args.hInstance = hInstance;
    args.hPrevInstance = hPrevInstance;
    args.lpCmdLine = lpCmdLine;
    args.nShowCmd = nShowCmd;

    Application &app = *create_app();

    app.init();
    app.run();

#ifdef HKDEBUG
    deallocWinConsole();
    removeLogFile();
#endif

    return 0;
}

#endif // HK_WINMAIN_H
