#include "entry.h"

#include "Tests.h"

Application* create_app()
{
    AppDesc desc;
    desc.title = "Tests";
    desc.width = 0;
    desc.height = 0;

    return new Tests(desc);
}
