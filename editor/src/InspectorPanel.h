#ifndef HK_INSPECTOR_PANEL_H
#define HK_INSPECTOR_PANEL_H

#include "hikai.h"

class InspectorPanel {
public:
    void init(GUI *gui);

    void display(u32 hdnl);

private:
    void addTransform(Transform &tr);
    void addMaterials(hk::Model *model);

private:
    GUI *gui;
    // u32 hndlSelectedModel;
};

#endif // HK_INSPECTOR_PANEL_H
