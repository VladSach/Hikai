#ifndef HK_RESOURCES_PANEL_H
#define HK_RESOURCES_PANEL_H

#include "hikai.h"

class ResourcesPanel {
public:
    void init(Renderer *renderer);
    void deinit();

    void display();

public:
    b8 is_open_;

    Renderer *renderer_;
};

#endif // HK_RESOURCES_PANEL_H

