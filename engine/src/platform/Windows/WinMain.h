#include "core/Application.h"
#include "platform/platform.h"
#include "platform/PlatformArgs.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nShowCmd)
{
    // (void)nShowCmd; (void)lpCmdLine; (void)hPrevInstance;

#ifdef HKDEBUG
    WinLog log;
    log.allocWinConsole();
#endif

    LOG_INFO("Initializing Windows startup");

    PlatformArgs &args = *PlatformArgs::instance();
    args.hInstance = hInstance;
    args.hPrevInstance = hPrevInstance;
    args.lpCmdLine = lpCmdLine;
    args.nShowCmd = nShowCmd;

    Application &app = *create_app();
    app.setWindow();

    app.init();
    app.run();

#ifdef HKDEBUG
    log.deallocWinConsole();
#endif

    return 0;
}
