#ifndef HK_WINMAIN_H
#define HK_WINMAIN_H

#include "hkstl/Logger.h"
#include "core/Application.h"
#include "platform/args.h"
#include "platform/platform.h"
#include "platform/Windows/WinLog.h"

int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nShowCmd)
{
    hk::log::init();

#ifdef HKDEBUG
    allocWinConsole();
    setLogFile("hikai_log.txt");
#endif

    LOG_INFO("Initializing Windows startup");

    hk::platform::args::hInstance     = hInstance;
    hk::platform::args::hPrevInstance = hPrevInstance;
    hk::platform::args::lpCmdLine     = lpCmdLine;
    hk::platform::args::nShowCmd      = nShowCmd;

    Application *app = create_app();

    app->init();
    app->run();
    app->deinit();

    delete app;

#ifdef HKDEBUG
    removeLogFile();
    deallocWinConsole();
#endif

    hk::log::deinit();
    return 0;
}

#endif // HK_WINMAIN_H
