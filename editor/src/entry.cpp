#include "entry.h"

#include "Editor.h"

Application* create_app()
{
    AppDesc desc;
    desc.title = "Hikai Editor";
    desc.width = 1600;
    desc.height = 860;

    return new Editor(desc);
}
