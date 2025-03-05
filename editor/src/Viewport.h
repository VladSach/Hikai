#ifndef HK_VIEWPORT_H
#define HK_VIEWPORT_H

#include "hikai.h"

#include "renderer/ui/imguiwrapper.h"

class Viewport {
public:
    void init(Renderer *renderer, Camera *camera, b8 viewport);
    void deinit();

    void display(hk::SceneNode *selected, void *image);

private:
    void processInput();

    void showControls();
    void showGizmo(hk::SceneNode *selected);

public:
    constexpr b8 isMouseOverViewport() const { return is_mouse_over_viewport_; }

private:
    Camera *camera_;
    Renderer *renderer_;

    b8 is_mouse_over_viewport_ = false;
    b8 is_viewport_;

    // Gizmo controls
    b8 use_snap_ = false;
    f32 snap_value_ = 10;
    ImGuizmo::OPERATION gizmo_op_   = ImGuizmo::TRANSLATE;
    ImGuizmo::MODE      gizmo_mode_ = ImGuizmo::LOCAL;
};

#endif // HK_VIEWPORT_H
