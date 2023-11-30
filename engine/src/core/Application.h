#ifndef HK_APPLICATION_H
#define HK_APPLICATION_H

#include "defines.h"
#include "Timer.h"
#include "EventSystem.h"
#include "platform/platform.h"

struct AppDesc {
    u32 width = 400;
    u32 height = 400;
    std::wstring title = L"Sandbox";
};

// TODO: find where to put deinits (window, logger, eventsystem, input, ...)
class Application {
private:
    b8 initialized = false;

    AppDesc desc;
    Window window;
    EventSystem *evsys;

    hk::Timer clock;
public:
    HKAPI Application(const AppDesc &desc);
    HKAPI virtual ~Application() {};

    // OPTIMIZE: Maybe add basic implementation instead of pure abstract?

    // Being called before anything else, right after app creation
    HKAPI virtual void init() = 0;
    // Being called every frame
    HKAPI virtual void update(f32 dt) { (void)dt; };
    // Being called by fixed amount of time
    HKAPI virtual void fixedUpdate() {};
    // Being called at the end of the game loop
    HKAPI virtual void render() = 0;

    HKAPI const AppDesc& getDesc() const { return desc; }

    // Engine internal use
    HKAPI void run();
};

// Defined by user
Application* create_app();

#endif // HK_APPLICATION_H
