#include "Application.h"

void Application::run()
{
    bool running = true;
    while (running) {

        if (!window.ProcessMessages())
            running = false;
    }
}

void Application::setWindow()
{
    window.init(desc.title, desc.width, desc.height);
}
