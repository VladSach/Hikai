#include "Application.h"

#include "input.h"

#include <thread>

Application::Application(const AppDesc &desc)
    : desc(desc)
{
    evsys = EventSystem::instance();

    window.init(desc.title, desc.width, desc.height);
    evsys->init();
    hk::input::init();

    clock.record();

    renderer.init(desc.renderBackend);
}

void Application::run()
{
    f32 dt = .0f;
    const f32 frameRate = 1.0f / 60.f;

    b8 running = true;
    while (running) {

        if (!window.ProcessMessages()) {
            running = false;
        }

        dt += static_cast<f32>(clock.update());
        if (!window.getIsVisible() || dt < frameRate) {
            std::this_thread::yield();
            dt += static_cast<f32>(clock.update());
        }

        evsys->dispatch();

        update(dt);

        hk::input::update();

        render();
    }
}

