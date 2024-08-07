#include "Editor.h"

void Editor::init()
{
    LOG_INFO("Editor created");

    window_ = getDesc().window;

    f32 aspect = static_cast<f32>(window_->getWidth())/window_->getHeight();

    camera_.setPerspective(45.f, aspect, .1f, 100.f);
    camera_.setWorldOffset({ 0.f, 0.f, -2.f });
}

void Editor::update(f32 dt)
{
    f32 time = .0f;

    processInput(dt);

    hkm::mat4f mat = camera_.getViewProjection();
    hk::ubo::setFrameData({
        {
            static_cast<f32>(window_->getWidth()),
            static_cast<f32>(window_->getHeight())
        },
        time,
        {},
        mat,
    });

    time += dt;
}

void Editor::processInput(f32 dt)
{
    if (hk::input::isKeyDown(hk::input::Button::KEY_F11))
        renderer->toggleUIMode();

    // FIX: temp
    if (renderer->ui().isInputLocked()) {
        return;
    }

    hkm::vec3f offset;
    hkm::vec3f angles;
    hkm::vec3f direction;

    if (hk::input::isKeyDown(hk::input::Button::KEY_A))
        offset += {-1.f, 0.f, 0.f};
    if (hk::input::isKeyDown(hk::input::Button::KEY_D))
        offset += {1.f, 0.f, 0.f};
    if (hk::input::isKeyDown(hk::input::Button::KEY_W))
        offset += {0.f, 0.f, 1.f};
    if (hk::input::isKeyDown(hk::input::Button::KEY_S))
        offset += {0.f, 0.f, -1.f};
    if (hk::input::isKeyDown(hk::input::Button::KEY_SPACE))
        offset += {0.f, 1.f, 0.f};
    if (hk::input::isKeyDown(hk::input::Button::KEY_LCTRL))
        offset += {0.f, -1.f, 0.f};

    if (hk::input::isKeyDown(hk::input::Button::KEY_Q))
        angles = {0.f, 0.f, 1.f};
    if (hk::input::isKeyDown(hk::input::Button::KEY_E))
        angles = {0.f, 0.f, -1.f};

    if (hk::input::isMouseDown(hk::input::Button::BUTTON_RIGHT)) {
        window_->hideCursor();
        window_->lockCursor();

        i32 pitch, yaw;
        hk::input::getMouseDelta(yaw, pitch);
        if (pitch != 0 || yaw != 0) {
            pitch = hkm::clamp(pitch, -1, 1);
            yaw = hkm::clamp(yaw, -1, 1);

            angles = {
                static_cast<f32>(-yaw),
                static_cast<f32>(-pitch), 0
            };

        }

        // i32 x, y;
        // hk::input::getMousePosition(x, y);
        //
        // f32 width  = static_cast<f32>(window->getWidth());
        // f32 height = static_cast<f32>(window->getHeight());
        // f32 u = (static_cast<f32>(x) + .5f) / width;
        // f32 v = (static_cast<f32>(y) + .5f) / height;
        //
        // u = 2.f * u - 1.f;
        // v = 2.f * v - 1.f;
        //
        // direction = normalize(
        //     transformPoint(
        //         mainCamera.getInvViewProjection(),
        //         {u, v, 0}
        //     )
        // );
        //
        // mainCamera.lookAt(direction);
    }

    if (hk::input::isMouseReleased(hk::input::Button::BUTTON_RIGHT)) {
        window_->showCursor();
        window_->unlockCursor();
    }

    constexpr f32 rotationSpeed = 45.f;
    camera_.addRelativeAngles(angles * dt * rotationSpeed);
    camera_.addRelativeOffset(offset * dt);
    camera_.update();
}

void Editor::render()
{

}
