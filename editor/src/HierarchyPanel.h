#ifndef HK_HIERARCHY_PANEL_H
#define HK_HIERARCHY_PANEL_H

#include "hikai.h"

class HierarchyPanel {
public:
    void init();

    void display(GUI &gui);

    inline u32 selectedAssetHandle() const { return hndlSelectedAsset; }

private:
    hk::vector<hk::ModelAsset*> models;

private:
    u32 hndlSelectedAsset = 0;
};

#endif // HK_HIERARCHY_PANEL_H
