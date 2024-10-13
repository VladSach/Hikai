#ifndef HK_APPLICATION_H
#define HK_APPLICATION_H

#include "defines.h"
#include "Timer.h"
#include "EventSystem.h"
#include "SceneGraph.h"
#include "platform/platform.h"
#include "renderer/Renderer.h"

struct AppDesc {
    u32 width = 400;
    u32 height = 400;
    std::wstring title = L"Sandbox";

    Window *window = nullptr;
};

class Application {
public:
    HKAPI Application(const AppDesc &desc);
    HKAPI virtual ~Application() { deinit(); };

    // Being called before anything else, right after app creation
    HKAPI virtual void init() {};
    // Being called every frame
    HKAPI virtual void update(f32 dt) { (void)dt; };
    // Being called by fixed amount of time
    HKAPI virtual void fixedUpdate() {};
    // Being called at the end of the game loop
    HKAPI virtual void render() {};

    HKAPI const AppDesc& getDesc() const { return desc; }

    // Engine internal use
    HKAPI void run();

    void deinit();

    static void shutdown(hk::EventContext errorCode, void*);
    static b8 running;

protected:
    hk::SceneGraph scene;
    Renderer *renderer;
    Window *window;

private:
    b8 initialized = false;

    AppDesc desc;
    hk::EventSystem *evsys;

    hk::Timer clock;
};

// Defined by user
Application* create_app();

#endif // HK_APPLICATION_H
