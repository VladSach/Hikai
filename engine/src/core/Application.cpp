#include "Application.h"

#include "input.h"
#include "utils/Filewatch.h"
#include "resources/AssetManager.h"

#include <thread>

b8 Application::running = false;

Application::Application(const AppDesc &desc)
    : desc(desc)
{
    evsys = EventSystem::instance();

    evsys->init();
    evsys->subscribe(hk::EVENT_APP_SHUTDOWN, shutdown, this);

    window = desc.window;
    if (!window) {
        allocWinConsole();
        window = new EmptyWindow();
        return;
    }

    window->init(desc.title, desc.width, desc.height);
    window->enableRawMouseInput();

    hk::input::init();

    clock.record();

    hk::assets()->init("assets\\");

    renderer = new Renderer();
    renderer->init(window);

    running = true;
}

void Application::run()
{
    f32 dt = .0f;
    const f32 desiredFrameRate = 60.f; // TODO: configurable
    const f32 msPerFrame = 1.0f / desiredFrameRate;

    f32 fixedTimestamp = .0f;

    while (running) {
        if (!window->ProcessMessages()) {
            running = false;
            break;
        }

        dt = static_cast<f32>(clock.update());

        if (!window->getIsVisible()) {
            continue;
        }

        // PERF: is spin loop faster then waiting for (frameRate - dt)?
        while (dt < msPerFrame) {
            std::this_thread::sleep_for(std::chrono::nanoseconds(1));
            dt += static_cast<f32>(clock.update());
        }

        update(dt);
        hk::input::update();

        evsys->dispatch();

        fixedTimestamp += dt;
        while (fixedTimestamp >= msPerFrame) {
            fixedUpdate();
            fixedTimestamp -= msPerFrame;
        }

        render();
        renderer->draw();
    }
}

void Application::deinit()
{
    hk::assets()->deinit();
    hk::filewatch::deinit();
    renderer->deinit();
    hk::input::deinit();
    window->deinit();
    evsys->deinit();
}

void Application::shutdown(hk::EventContext errorCode, void* listener)
{
    (void)listener;

    if (errorCode.u64) {
        hk::ErrorCode err = static_cast<hk::ErrorCode>(errorCode.u64);
        LOG_FATAL("Error:", err, "-", hk::getErrocodeStr(err));

        // TODO: probably should also create a Message Box,
        // so user can understand why application closed
    }

    // TODO: not the best way to quit application immediately
    // at least one loop iteration will happen after application
    // was supposed to crash
    // and in init stage all subsequent to crash init calls will also be called
    running = false;
}
