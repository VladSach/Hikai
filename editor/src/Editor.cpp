#include "Editor.h"

#include "renderer/ui/imguiwrapper.h"

#include "utils/Filewatch.h"
#include "utils/thumbnails.h"

void Editor::init()
{
    LOG_INFO("Editor created");

    f32 aspect = static_cast<f32>(window_->width())/window_->height();

    camera_.setPerspective(60.f, aspect, .01f, 100.f);
    camera_.setWorldOffset({ 0.f, 0.5f, -1.5f });

    viewportMode = true;

    restoreLayout();

    hke::thumbnail::init(renderer_);

    viewport.init(renderer_, &camera_, viewportMode);

    assets.init();
    hierarchy.init(&scene_);
    inspector.init();
    log.init();
    metrics.init();
    settings.init(renderer_);

    hkm::quaternion rot = hkm::fromAxisAngle({0.f, 1.f, 0.f}, 0.f * hkm::degree2rad);

    u32 handle = 0;
    handle = hk::assets()->load("Rei Plush.fbx");
    scene_.addModel(handle, { 0.f, .001f, rot });

    handle = hk::assets()->load("Knight_All.fbx");
    scene_.addModel(handle, { {1.5f, 0.f, 0.f}, .001f, rot });

    handle = hk::assets()->load("Samurai.fbx");
    scene_.addModel(handle, { {-1.5f, 0.f, 0.f}, 1.f, rot });

    handle = hk::assets()->load("warrior.fbx");
    scene_.addModel(handle, { {.0f, 0.f, 1.5f}, .001f, rot });

    handle = hk::assets()->load("sponza.obj");
    scene_.addModel(handle, { {.0f, 0.f, .0f}, .01f, rot });

    hk::Light light;
    light.type = hk::Light::Type::POINT_LIGHT;
    light.color = {0.823f, 0.760f, 0.635f, 1.f};
    light.intensity = 1.f;
    scene_.addLight(light, {{0.1f, 0.8f, 0.3f}, 1.f, rot});

    light.type = hk::Light::Type::SPOT_LIGHT;
    light.color = {0.0f, 0.0, 1.0, 1.f};
    light.inner_cutoff = 6.f;
    light.outer_cutoff = 9.f;
    scene_.addLight(light, {{0.1f, 0.8f, 0.3f}, 1.f, rot});

    light.type = hk::Light::Type::DIRECTIONAL_LIGHT;
    light.color = {1.0f, 0.0, 0.0, 1.f};
    scene_.addLight(light, {{0.1f, 0.8f, 0.3f}, 1.f, rot});
}

void Editor::deinit()
{
    // settings.deinit();
    // metrics.deinit();
    // log.deinit();
    // hierarchy.deinit();
    // inspector.deinit();
    // assets.deinit();
    // viewport.deinit();
}

void Editor::update(f32 dt)
{
    processInput(dt);
}

void Editor::render()
{
    hk::imgui::push([&](){
        if (!viewportMode) { return; }

        mainDockSpaceID =
            ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());
    });

    showMenuBar();

    viewport.display(selected, settings.viewport_image_);

    assets.display();
    hierarchy.display();
    selected = hierarchy.selectedNode();
    inspector.display(selected);
    log.display();
    metrics.display();
    settings.display();

    if (showImGuiDemo) {
        hk::imgui::push([&](){
            ImGui::ShowDemoWindow();
        });
    }
}

void Editor::showMenuBar()
{
    hk::imgui::push([&](){
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("Options")) {

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Debug")) {

                ImGui::MenuItem("ImGui Demo Window", NULL, &showImGuiDemo);

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Windows")) {

                ImGui::MenuItem("Asset Browser", NULL, &assets.is_open_);
                ImGui::MenuItem("Hierarchy",     NULL, &hierarchy.is_open_);
                ImGui::MenuItem("Inspector",     NULL, &inspector.is_open_);
                ImGui::MenuItem("Log",           NULL, &log.is_open_);
                ImGui::MenuItem("Metrics",       NULL, &metrics.is_open_);
                ImGui::MenuItem("Settings",      NULL, &settings.is_open_);

                if (ImGui::MenuItem("RestoreLayout")) {
                    // FIX: doesn't work
                    restoreLayout();
                }

                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }
    });
}

void Editor::restoreLayout()
{
    if (!viewportMode) { return; }

    hk::imgui::push([&](){
        mainDockSpaceID =
            ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());

        ImGui::DockBuilderRemoveNode(mainDockSpaceID);
        ImGui::DockBuilderAddNode(mainDockSpaceID);

        upper = mainDockSpaceID;
        right = ImGui::DockBuilderSplitNode(upper, ImGuiDir_Right, 0.20f, nullptr, &upper);
        lower = ImGui::DockBuilderSplitNode(upper, ImGuiDir_Down,  0.30f, nullptr, &upper);
        left  = ImGui::DockBuilderSplitNode(upper, ImGuiDir_Left,  0.20f, nullptr, &upper);

        ImGui::DockBuilderDockWindow("Viewport",  upper);
        ImGui::DockBuilderDockWindow("Assets",    lower);
        ImGui::DockBuilderDockWindow("Log",       lower);
        ImGui::DockBuilderDockWindow("Scene",     left);
        ImGui::DockBuilderDockWindow("Inspector", right);

        ImGui::DockBuilderFinish(mainDockSpaceID);
    });
}

void Editor::processInput(f32 dt)
{
    if (hk::input::isKeyPressed(hk::input::Button::KEY_F11)) {
        viewportMode = false;
    }

    wasInViewport = isInViewport;
    isInViewport = !viewport.isMouseOverViewport();

    if (!isInViewport && !wasInViewport) { return; }

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

    if (hk::input::isKeyDown(hk::input::Button::KEY_LSHIFT))
        offset *= 4.f;

    if (hk::input::isKeyDown(hk::input::Button::KEY_Q))
        angles = {0.f, 0.f, -1.f};
    if (hk::input::isKeyDown(hk::input::Button::KEY_E))
        angles = {0.f, 0.f, 1.f};

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

        isInViewport = wasInViewport ? true : false;

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

    constexpr f32 rotationSpeed = 75.f;
    hkm::vec3f zrot = {0.f, 0.f, angles.z};
    hkm::vec2f bottomrot = {angles.x, angles.y};
    camera_.addRelativeAngles(zrot * dt * rotationSpeed);
    camera_.fixedBottomRotation(bottomrot * dt * rotationSpeed);
    camera_.addRelativeOffset(offset * dt);
    camera_.update();
}
