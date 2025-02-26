#include "Viewport.h"

void Viewport::init(Renderer *renderer, Camera *camera, b8 viewport)
{
    camera_ = camera;
    renderer_ = renderer;

    is_viewport_ = viewport;

    use_post_process_ = false;

    hk::event::subscribe(hk::event::EVENT_WINDOW_RESIZE,
        [&](const hk::event::EventContext &context, void *listener) {
            (void)context; (void)listener;

            // if (viewport_image_) {
            //     hk::imgui::removeTexture(viewport_image_);
            // }

            post_process_ =
                hk::imgui::addTexture(renderer_->post_process_.color_.view(),
                                      renderer_->samplerLinear);
            offscreen_ =
                hk::imgui::addTexture(renderer_->offscreen_.color_.view(),
                                      renderer_->samplerLinear);
    }, this);
}

void Viewport::deinit()
{
    // hk::event::unsubscribe(hk::event::EVENT_WINDOW_RESIZE, this);
}

void Viewport::display(hk::SceneNode *selected)
{
    viewport_image_ = use_post_process_ ? post_process_ : offscreen_;

    hk::imgui::push([&](){
        ImGuiWindowFlags flags = ImGuiWindowFlags_None;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {.0f, .0f});

        if (!is_viewport_) {
            f32 menu_height = ImGui::FindWindowByName("##MainMenuBar")->Size.y;

            ImGuiIO io = ImGui::GetIO();
            ImGui::SetNextWindowSize({io.DisplaySize.x, io.DisplaySize.y - menu_height});
            ImGui::SetNextWindowPos({.0f, menu_height});
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, .0f);

            flags = ImGuiWindowFlags_NoDecoration |
                    ImGuiWindowFlags_NoResize |
                    ImGuiWindowFlags_NoBringToFrontOnFocus |
                    ImGuiWindowFlags_None;
        }

        if (ImGui::Begin("Viewport", 0, flags)) {
            is_mouse_over_viewport_ = !ImGui::IsWindowHovered();

            processInput();

            ImVec2 panel_size = ImGui::GetContentRegionAvail();
            ImGui::Image(viewport_image_, ImVec2{ panel_size.x, panel_size.y });

            showControls();

            showGizmo(selected);

        } ImGui::End();

        if (!is_viewport_) { ImGui::PopStyleVar(); }

        ImGui::PopStyleVar();
    });
}

void Viewport::processInput()
{
    // Maybe move to Editor
    if (ImGui::IsKeyPressed(ImGuiKey_T)) { gizmo_op_ = ImGuizmo::TRANSLATE; }
    if (ImGui::IsKeyPressed(ImGuiKey_R)) { gizmo_op_ = ImGuizmo::ROTATE; }
    if (ImGui::IsKeyPressed(ImGuiKey_Y)) { gizmo_op_ = ImGuizmo::SCALE; }

    use_snap_ = false;
    if (ImGui::IsKeyPressed(ImGuiKey_LeftShift)) {
        use_snap_ = true;
    }
}

void Viewport::showControls()
{
    ImGui::SetNextItemAllowOverlap();
    ImGui::SetCursorPos(ImGui::GetWindowContentRegionMin());

    ImGui::BeginGroup();
        if (ImGui::Button("Translate")) { gizmo_op_ = ImGuizmo::TRANSLATE; }
        ImGui::SameLine();
        if (ImGui::Button("Scale"))     { gizmo_op_ = ImGuizmo::SCALE; }
        ImGui::SameLine();
        if (ImGui::Button("Rotate"))    { gizmo_op_ = ImGuizmo::ROTATE; }
    ImGui::EndGroup();
}

void Viewport::showGizmo(hk::SceneNode *selected)
{
    ImGuiWindow *viewport = ImGui::GetCurrentWindow();
    ImGuizmo::SetRect(viewport->Pos.x, viewport->Pos.y,
                      viewport->Size.x, viewport->Size.y);
    ImGuizmo::SetAlternativeWindow(viewport);

    if (selected) {
        // ImGuizmo accepts LH coordinate system
        // Quick fix for RH to LH, maybe there is another way
        hkm::mat4f proj = camera_->projection(); proj(1, 1) *= -1;

        f32 bounding_size = .0f;
        hkm::mat4f matrix = selected->world.toMat4f();

        if (ImGuizmo::Manipulate(*camera_->view().n, *proj.n,
                                 gizmo_op_, gizmo_mode_,
                                 *matrix.n, NULL,
                                 use_snap_ ? &snap_value_ : NULL,
                                 bounding_size ? &bounding_size : NULL))
        {
            hkm::mat4f parentInv =
                hkm::inverse(selected->parent->world.toMat4f());

            // FIX: ffs, why they work in wrong order(
            selected->local = Transform(matrix * parentInv);
            selected->dirty = true;
        }
    }
}
