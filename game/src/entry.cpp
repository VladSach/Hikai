#include "entry.h"

class Blight : public Application {
public:
    Blight(const AppDesc &desc)
        : Application(desc) {}

    void init()
    {
        LOG_INFO("Test");
        LOG_WARN("Test");
        LOG_ERROR("Test");
        LOG_FATAL("Test");
        LOG_DEBUG("Test");
        LOG_TRACE("Test");
    }

    void update(f32 dt) {}
    void render() {}
};

Application* create_app()
{
    AppDesc desc;
    desc.title = L"Blight";

    return new Blight(desc);
}
