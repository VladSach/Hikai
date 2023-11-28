#include "Application.h"
#include "input.h"

Application::Application(const AppDesc &desc)
    : desc(desc)
{
    evsys = EventSystem::instance();

    window.init(desc.title, desc.width, desc.height);
    evsys->init();
    hk::input::init();

}

void Application::run()
{
    b8 running = true;
    while (running) {
        if (!window.ProcessMessages()) {
            running = false;
        }

        evsys->dispatch();

        update(0);
        hk::input::update();

        render();
    }
}
