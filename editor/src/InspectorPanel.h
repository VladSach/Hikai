#ifndef HK_INSPECTOR_PANEL_H
#define HK_INSPECTOR_PANEL_H

#include "hikai.h"

class InspectorPanel {
public:
    void init(GUI *gui);

    void display(hk::SceneNode *node);

private:
    void addBasicAssetProperties(hk::SceneNode *node);
    void addTransform(hk::SceneNode *node);

    // Entity Info
    void addMeshInfo(const hk::MeshAsset &mesh);
    void addMaterialInfo(hk::SceneNode *node);

private:
    GUI *gui;

    hk::vector<hk::MaterialAsset*> materials;
};

#endif // HK_INSPECTOR_PANEL_H
