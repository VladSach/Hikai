#ifndef HK_APPLICATION_H
#define HK_APPLICATION_H

#include "hkcommon.h"
#include "events.h"
#include "Clock.h"
#include "SceneGraph.h"
#include "platform/platform.h"
#include "renderer/Renderer.h"
#include "renderer/object/Camera.h"

#include "hkstl/Logger.h"

#include <string>

struct AppDesc {
    u32 width = 400;
    u32 height = 400;
    std::string title = "Sandbox";
};

class Application {
public:
    HKAPI Application(const AppDesc &desc);
    HKAPI virtual ~Application() { cleanup(); };

    // Being called before anything else, right after app creation
    HKAPI virtual void init() {};
    // Being called every frame
    HKAPI virtual void update(f32 dt) { (void)dt; };
    // Being called by fixed amount of time
    HKAPI virtual void fixedUpdate() {};
    // Being called at the end of the game loop
    HKAPI virtual void render() {};
    // Being called first thing at app shutdown
    HKAPI virtual void deinit() {};

    // Engine internal use
    HKAPI void run();

    // void startup();
    void cleanup();

    static void shutdown(hk::event::EventContext ctx, void*);
    static b8 running;

protected:
    hk::SceneGraph scene_;
    Renderer *renderer_;
    Window *window_;

    AppDesc desc_;
    const f32 desired_frame_rate_ = 60.f;

    hk::Camera camera_;

private:
    b8 is_initialized_ = false;

    hk::Clock clock_;

    u32 cntr_loop_ = 0;
    f32 time_since_start_ = .0f;
};

// Defined by user
Application* create_app();

#endif // HK_APPLICATION_H
