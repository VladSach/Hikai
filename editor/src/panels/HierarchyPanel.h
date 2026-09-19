#ifndef HK_HIERARCHY_PANEL_H
#define HK_HIERARCHY_PANEL_H

#include "hikai.h"

class HierarchyPanel {
public:
    void init(hk::SceneGraph *scene);

    void display();

    void controls();

public:
    constexpr hk::SceneNode* selectedNode() const { return selected_; }

public:
    b8 is_open_;

private:
    u32 node_clicked = static_cast<u32>(-1);

    hk::SceneGraph *scene_;
    hk::SceneNode *selected_ = nullptr;

};

#endif // HK_HIERARCHY_PANEL_H
