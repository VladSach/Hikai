#ifndef HK_WINMAIN_H
#define HK_WINMAIN_H

#include "hkstl/Logger.h"
#include "core/Application.h"
#include "args.h"
#include "platform/platform.h"

int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nShowCmd)
{
    hk::platform::args::hInstance     = hInstance;
    hk::platform::args::hPrevInstance = hPrevInstance;
    hk::platform::args::lpCmdLine     = lpCmdLine;
    hk::platform::args::nShowCmd      = nShowCmd;

    hk::log::init();

#ifdef HKDEBUG
    hk::platform::alloc_console();
    // hk::platform::set_log_file("hikai_log.txt");
#endif

    LOG_INFO("Initializing Windows startup");

    Application *app = create_app();

    app->init();
    app->run();
    app->deinit();

    delete app;

#ifdef HKDEBUG
    // hk::platform::remove_log_file();
    hk::platform::dealloc_console();
#endif

    hk::log::deinit();
    return 0;
}

#endif // HK_WINMAIN_H
