#ifndef HK_LINUXMAIN_H
#define HK_LINUXMAIN_H

#include "hkstl/Logger.h"
#include "core/Application.h"

#include "args.h"
#include "platform/platform.h"
#include "platform/console/console.h"

int main(int argc, char **argv)
{
    hk::log::init();

#ifdef HKDEBUG
    hk::platform::alloc_console();
    // hk::platform::set_log_file("hikai_log.txt");
#endif

    LOG_INFO("Initializing Linux startup");

    hk::platform::args::argc = argc;
    hk::platform::args::argv = argv;

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

#endif // HK_LINUXMAIN_H
