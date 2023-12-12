#include "entry.h"

#include "Sandbox.h"

Application* create_app()
{
    AppDesc desc;
    desc.title = L"Sandbox";
    desc.width = 1020;
    desc.height = 780;
    desc.window = new WinWindow();

    return new Sandbox(desc);
}
