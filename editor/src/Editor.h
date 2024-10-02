#ifndef HK_EDITOR_H
#define HK_EDITOR_H

#include "hikai.h"

#include "AssetBrowser.h"
#include "InspectorPanel.h"
#include "HierarchyPanel.h"

class Editor final : public Application {
public:
    Editor(const AppDesc &desc)
        : Application(desc) {}

    ~Editor() {
        LOG_INFO("Editor is closed");
    }

    void init();
    void update(f32 dt);
    void render();

private:
    void processInput(f32 dt);

private:
    Window *window_ = nullptr;

    Camera camera_;

    f32 time_ = .0f;

// GUI part
private:
    AssetBrowser assets;
    InspectorPanel inspector;
    HierarchyPanel hierarchy;
};

#endif // HK_EDITOR_H
