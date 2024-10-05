#ifndef HK_INSPECTOR_PANEL_H
#define HK_INSPECTOR_PANEL_H

#include "hikai.h"

class InspectorPanel {
public:
    void init(GUI *gui);

    void display(u32 hdnl);

private:
    void addBasicAssetProperties(hk::Asset *asset);
    void addTransform(Transform &tr);
    void addMaterials(hk::Model *model);

private:
    GUI *gui;

    void *diffuseThumbnailGui;
};

#endif // HK_INSPECTOR_PANEL_H
