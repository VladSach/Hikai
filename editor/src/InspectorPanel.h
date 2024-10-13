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

    // Mesh Inspect
    void addMeshInfo(const hk::MeshAsset &mesh);
    void addMaterials(hk::Material *material);

    // Model Inspect

private:
    GUI *gui;

    void *diffuseThumbnailGui;
};

#endif // HK_INSPECTOR_PANEL_H
