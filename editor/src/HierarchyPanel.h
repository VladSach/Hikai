#ifndef HK_HIERARCHY_PANEL_H
#define HK_HIERARCHY_PANEL_H

#include "hikai.h"

class HierarchyPanel {
public:
    void init(hk::SceneGraph *scene, GUI *gui);

    void display();

    void controls();

public:
    constexpr hk::SceneNode* selectedNode() const { return selected_; }

private:
    GUI *gui;

    i32 node_clicked = -1;

    hk::SceneGraph *scene;
    hk::SceneNode *selected_ = nullptr;
};

#endif // HK_HIERARCHY_PANEL_H
