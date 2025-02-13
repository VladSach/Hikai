#ifndef HK_EDITOR_H
#define HK_EDITOR_H

#include "hikai.h"

#include "Viewport.h"

#include "AssetBrowser.h"
#include "panels/InspectorPanel.h"
#include "panels/HierarchyPanel.h"
#include "panels/LogPanel.h"
#include "panels/MetricsPanel.h"
#include "panels/SettingsPanel.h"

class Editor final : public Application {
public:
    Editor(const AppDesc &desc)
        : Application(desc) {}

    ~Editor() {
        LOG_INFO("Editor is closed");
    }

    void init();
    void deinit();

    void update(f32 dt);
    void render();

private:
    void processInput(f32 dt);

private:
    hk::SceneNode *selected = nullptr;

    b8 wasInViewport = false;
    b8 isInViewport = false;

    b8 viewportMode = true;

// GUI part
private:
    Viewport viewport;

    AssetBrowser assets;
    InspectorPanel inspector;
    HierarchyPanel hierarchy;
    LogPanel log;
    MetricsPanel metrics;
    SettingsPanel settings;

    b8 showImGuiDemo = false;

    ImGuiID mainDockSpaceID;
    ImGuiID upper;
    ImGuiID lower;
    ImGuiID left;
    ImGuiID right;

private:
    void showMenuBar();
    void restoreLayout();
};

#endif // HK_EDITOR_H
