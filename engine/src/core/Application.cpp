#include "Application.h"

#include "input.h"

#include "renderer/UBManager.h"

// FIX: temp
#include "renderer/object/Camera.h"

#include <thread>

b8 Application::running = false;

Application::Application(const AppDesc &desc)
    : desc(desc)
{
    evsys = EventSystem::instance();

    evsys->init();
    evsys->subscribe(hk::EVENT_APP_SHUTDOWN, shutdown);

    window = desc.window;
    if (!window) {
        allocWinConsole();
        window = new EmptyWindow();
        return;
    }

    window->init(desc.title, desc.width, desc.height);

    hk::input::init();

    clock.record();

    renderer = hk::device();
    renderer->init(window);

    running = true;
}

void Application::run()
{
    f32 dt = .0f;
    const f32 desiredFrameRate = 60.f; // TODO: configurable
    const f32 msPerFrame = 1.0f / desiredFrameRate;

    f32 fixedTimestamp = .0f;
    f32 time = .0f;

    // FIX: temp
    Camera mainCamera;
    f32 aspect = static_cast<f32>(window->getWidth())/window->getHeight();
    mainCamera.setPerspective(45.f, aspect, .1f, 100.f);
    mainCamera.setWorldOffset({ 0.f, 0.f, -2.f });
    hkm::mat4f model = {
        1, 0, 0, 0,
        0, 1, 0, 5,
        0, 0, 1, 0,
        0, 0, 0, 1,
    };

    while (running) {
        if (!window->ProcessMessages()) {
            running = false;
            break;
        }

        dt = static_cast<f32>(clock.update());

        if (!window->getIsVisible()) {
            continue;
        }

        // PERF: is spin loop faster then waiting for (frameRate - dt)?
        while (dt < msPerFrame) {
            std::this_thread::sleep_for(std::chrono::nanoseconds(1));
            dt += static_cast<f32>(clock.update());
        }

        update(dt);
        hk::input::update();

        evsys->dispatch();

        fixedTimestamp += dt;
        while (fixedTimestamp >= msPerFrame) {
            fixedUpdate();
            fixedTimestamp -= msPerFrame;
        }

        // FIX: temp controls here, move to editor app later
        hkm::vec3f offset;
        hkm::vec3f angles;
        hkm::vec3f direction;
        if (hk::input::isKeyDown(hk::input::Button::KEY_A))
            offset = {-1.f, 0.f, 0.f};
        if (hk::input::isKeyDown(hk::input::Button::KEY_D))
            offset = {1.f, 0.f, 0.f};
        if (hk::input::isKeyDown(hk::input::Button::KEY_W))
            offset = {0.f, 0.f, 1.f};
        if (hk::input::isKeyDown(hk::input::Button::KEY_S))
            offset = {0.f, 0.f, -1.f};
        if (hk::input::isKeyDown(hk::input::Button::KEY_SPACE))
            offset = {0.f, 1.f, 0.f};
        if (hk::input::isKeyDown(hk::input::Button::KEY_LCTRL))
            offset = {0.f, -1.f, 0.f};

        if (hk::input::isKeyDown(hk::input::Button::KEY_Q))
            angles = {0.f, 0.f, 1.f};
        if (hk::input::isKeyDown(hk::input::Button::KEY_E))
            angles = {0.f, 0.f, -1.f};

        if (hk::input::isMouseDown(hk::input::Button::BUTTON_LEFT)) {
            window->hideCursor();

            // i32 pitch, yaw;
            // hk::input::getMouseDelta(yaw, pitch);
            // if (pitch != 0 || yaw != 0) {
            //     pitch = hkm::clamp(pitch, -1, 1);
            //     yaw = hkm::clamp(yaw, -1, 1);
            //
            //     angles = {
            //         static_cast<f32>(-yaw),
            //         static_cast<f32>(pitch), 0
            //     };
            //
            // }

            i32 x, y;
            hk::input::getMousePosition(x, y);

            f32 width  = static_cast<f32>(window->getWidth());
            f32 height = static_cast<f32>(window->getHeight());
            f32 u = (static_cast<f32>(x) + .5f) / width;
            f32 v = (static_cast<f32>(y) + .5f) / height;

            u = 2.f * u - 1.f;
            v = 2.f * v - 1.f;

            direction = normalize(
                transformPoint(
                    mainCamera.getInvViewProjection(),
                    {u, v, 0}
                )
            );

            mainCamera.lookAt(direction);
        }

        if (hk::input::isMouseReleased(hk::input::Button::BUTTON_LEFT)) {
            window->showCursor();
        }

        mainCamera.addRelativeAngles(angles);
        mainCamera.addRelativeOffset(offset * 0.05f);
        mainCamera.update();

        time += dt;
        hkm::mat4f mat = mainCamera.getViewProjection();
        hk::ubo::setFrameData({
            {
                static_cast<f32>(window->getWidth()),
                static_cast<f32>(window->getHeight())
            },
            time,
            mat,
        });

        renderer->draw();
        render();
    }
}

void Application::deinit()
{
    renderer->deinit();
    hk::input::deinit();
    window->deinit();
    evsys->deinit();
}

void Application::shutdown(hk::EventContext errorCode)
{
    if (errorCode.u64) {
        hk::ErrorCode err = static_cast<hk::ErrorCode>(errorCode.u64);
        LOG_FATAL("Error:", err, "-", hk::getErrocodeStr(err));

        // TODO: probably should also create a Message Box,
        // so user can understand why application closed
    }

    // TODO: not the best way to quit application immediately
    // at least one loop iteration will happen after application
    // was supposed to crash
    // and in init stage all subsequent to crash init calls will also be called
    running = false;
}
