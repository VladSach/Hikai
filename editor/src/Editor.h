#ifndef HK_EDITOR_H
#define HK_EDITOR_H

#include "hikai.h"

#include "AssetBrowser.h"
#include "InspectorPanel.h"
#include "HierarchyPanel.h"
#include "LogPanel.h"

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

    f32 snapValue = 10;
    u32 selected = 0;

    b8 wasInViewport = false;
    b8 isInViewport = false;

// GUI part
private:
    AssetBrowser assets;
    InspectorPanel inspector;
    HierarchyPanel hierarchy;
    LogPanel log;
};

#endif // HK_EDITOR_H
