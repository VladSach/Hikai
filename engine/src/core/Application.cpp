#include "Application.h"

#include "input.h"
#include "utils/filewatch.h"
#include "resources/AssetManager.h"
#include "platform/filesystem.h"

#include <thread>

b8 Application::running = false;

Application::Application(const AppDesc &desc)
    : desc_(desc)
{
    clock_.record();

    hk::event::init();
    hk::event::subscribe(hk::event::EVENT_APP_SHUTDOWN, shutdown, this);

    window_ = new Window();
    window_->init(desc.title, desc.width, desc.height);
    window_->enableRawMouseInput();

    hk::input::init();

    // FIX: temp development fix
    hk::assets()->init(hk::filesystem::canonical("..\\editor\\assets"));

    scene_.init();

    renderer_ = new Renderer();
    renderer_->init(window_);

    running = true;
}

void Application::run()
{
    f32 dt = .0f;
    f32 fixed_dt = .0f;

    const f32 ms_per_frame = 1.f / desired_frame_rate_;

    hk::DrawContext ctx;

    while (running) {
        if (!window_->ProcessMessages()) {
            running = false;
            break;
        }

        dt = static_cast<f32>(clock_.update());

        if (!window_->isVisible()) {
            continue;
        }

        // PERF: is spin loop faster then waiting for (frameRate - dt)?
        while (dt < ms_per_frame) {
            std::this_thread::sleep_for(std::chrono::nanoseconds(1));
            dt += static_cast<f32>(clock_.update());
        }

        hkm::mat4f mat = camera_.viewProjection();
        renderer_->updateFrameData(
        {
            {
                static_cast<f32>(window_->width()),
                static_cast<f32>(window_->height())
            },
            0,
            camera_.position(),
            time_since_start_,
            mat,
        });

        time_since_start_ += dt;

        update(dt);
        hk::input::update();

        hk::event::dispatch(); // FIX: why dispatch is here?

        fixed_dt += dt;
        while (fixed_dt >= ms_per_frame) {
            fixedUpdate();
            fixed_dt -= ms_per_frame;
        }

        scene_.update();
        scene_.updateDrawContext(ctx, *renderer_);

        render();
        renderer_->draw(ctx);

        hk::log::dispatch();
    }
}

void Application::cleanup()
{
    hk::assets()->deinit();
    hk::filewatch::deinit();
    renderer_->deinit();
    hk::input::deinit();
    window_->deinit();
    hk::event::deinit();
}

void Application::shutdown(hk::event::EventContext ctx, void* listener)
{
    (void)listener;

    if (ctx.u64) {
        hk::event::ErrorCode err = static_cast<hk::event::ErrorCode>(ctx.u64);
        ALWAYS_ASSERT(0, "Error:", err, "-", hk::event::getErrorCodeStr(err));
    }

    // TODO: not the best way to quit application immediately
    // at least one loop iteration will happen after application
    // was supposed to crash
    // and in init stage all subsequent to crash init calls will also be called
    running = false;
}
