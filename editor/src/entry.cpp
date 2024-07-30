#include "entry.h"

#include "Editor.h"

Application* create_app()
{
    AppDesc desc;
    desc.title = L"Hikai Editor";
    desc.width = 1020;
    desc.height = 780;
    desc.window = new WinWindow();

    return new Editor(desc);
}
