#include "Editor.h"

#include "utils/Filewatch.h"

void Editor::init()
{
    LOG_INFO("Editor created");

    window_ = getDesc().window;

    f32 aspect = static_cast<f32>(window_->getWidth())/window_->getHeight();

    camera_.setPerspective(60.f, aspect, .1f, 100.f);
    camera_.setWorldOffset({ 0.f, 0.5f, -1.5f });

    assets.init(renderer->ui());
    hierarchy.init();
    inspector.init(&renderer->ui());
    log.init(&renderer->ui());

    hkm::quaternion rot = hkm::fromAxisAngle({0.f, 1.f, 0.f}, 180.f * hkm::degree2rad);

    u32 handle = 0;
    handle = hk::assets()->load("Rei Plush.fbx");
    hk::assets()->getModel(handle).model->transform_ = {
        0.f, { .1f, .1f, .1f }, rot
    };
    handle = hk::assets()->load("Knight_All.fbx");
    hk::assets()->getModel(handle).model->transform_ = {
        {1.5f, 0.f, 0.f}, { .005f, .005f, .005f }, rot
    };
}

void Editor::update(f32 dt)
{
    processInput(dt);

    hkm::mat4f mat = camera_.viewProjection();
    hk::ubo::setFrameData(
    {
        {
            static_cast<f32>(window_->getWidth()),
            static_cast<f32>(window_->getHeight())
        },
        time_,
        camera_.position(),
        mat,
    });

    time_ += dt;
}

void Editor::processInput(f32 dt)
{
    if (hk::input::isKeyPressed(hk::input::Button::KEY_F11))
        renderer->toggleUIMode();

    wasInViewport = isInViewport;
    isInViewport = !renderer->ui().isInputLocked();

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

void Editor::render()
{
    assets.display(renderer->ui());
    hierarchy.display(renderer->ui());
    selected = hierarchy.selectedAssetHandle();
    inspector.display(selected);
    log.display();

    renderer->ui().pushCallback([&](){
        // TODO: move to gui utils
        auto drawMatrix4x4 = [](hkm::mat4f matrix){
            ImGuiTableFlags flags = ImGuiTableFlags_RowBg;
            flags |= ImGuiTableFlags_BordersOuter;
            if (ImGui::BeginTable("Matrix", 4, flags)) {
                for (u32 i = 0; i < 4; ++i) {
                    ImGui::TableNextRow();
                    for (u32 j = 0; j < 4; ++j) {
                        ImGui::TableSetColumnIndex(j);
                        ImGui::Text("%f", matrix(i, j));
                    }
                }
                ImGui::EndTable();
            }
        };

        static ImGuizmo::OPERATION gizmoOp   = ImGuizmo::TRANSLATE;
        static ImGuizmo::MODE      gizmoMode = ImGuizmo::WORLD;

        ImGuiWindow *viewport = ImGui::FindWindowByName("Viewport");
        ImGuizmo::SetRect(viewport->Pos.x, viewport->Pos.y,
                          viewport->Size.x, viewport->Size.y);
        ImGuizmo::SetAlternativeWindow(viewport);

        // TODO: move to process input
        if (ImGui::IsKeyPressed(ImGuiKey_T))
            gizmoOp = ImGuizmo::TRANSLATE;
        if (ImGui::IsKeyPressed(ImGuiKey_R))
            gizmoOp = ImGuizmo::ROTATE;
        if (ImGui::IsKeyPressed(ImGuiKey_Y))
            gizmoOp = ImGuizmo::SCALE;
        b8 useSnap = false;
        //if (hk::input::isKeyDown(hk::input::KEY_LSHIFT)) {
        if (ImGui::IsKeyPressed(ImGuiKey_LeftShift)) {
            useSnap = true;
        }


        if (ImGui::Begin("Viewport")) {
            if (selected) {
                hk::Asset *asset = hk::assets()->get(selected);
                if (asset->type != hk::Asset::Type::MODEL) { return; }

                hk::ModelAsset *model = reinterpret_cast<hk::ModelAsset*>(asset);
                hkm::mat4f matrix = model->model->transform_.toMat4f();
                // ImGuizmo accepts LH coordinate system
                // Quick fix for RH to LH, maybe there is another way
                hkm::mat4f proj = camera_.projection(); proj(1, 1) *= -1;
                ImGuizmo::Manipulate(*camera_.view().n, *proj.n,
                                    gizmoOp, gizmoMode, *matrix.n, NULL, useSnap ? &snapValue : NULL);
                model->model->transform_ = Transform(matrix);
            }

            ImGui::SetItemAllowOverlap();
            ImGui::SetCursorPos(ImGui::GetWindowContentRegionMin());
            ImGui::BeginGroup();
                if (ImGui::Button("Translate"))
                    gizmoOp = ImGuizmo::TRANSLATE;
                ImGui::SameLine();
                if (ImGui::Button("Scale"))
                    gizmoOp = ImGuizmo::SCALE;
                ImGui::SameLine();
                if (ImGui::Button("Rotate"))
                    gizmoOp = ImGuizmo::ROTATE;
                // if (ImGui::Button("Universal"))
                //     gizmoOp = ImGuizmo::UNIVERSAL;
            ImGui::EndGroup();

        } ImGui::End();
    });
}
