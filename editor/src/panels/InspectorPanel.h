#ifndef HK_INSPECTOR_PANEL_H
#define HK_INSPECTOR_PANEL_H

#include "hikai.h"

class InspectorPanel {
public:
    void init();

    void display(hk::SceneNode *node);

private:
    void addBasicAssetProperties(hk::SceneNode *node);
    void addTransform(hk::SceneNode *node);

    // Entity Info
    void addMeshInfo(const hk::MeshAsset &mesh);
    void addMaterialInfo(hk::SceneNode *node);
    void addLightInfo(hk::SceneNode *node);
    void addCameraInfo(hk::SceneNode *node);

public:
    b8 is_open_;

private:
    hk::vector<hk::MaterialAsset*> materials;
};

#endif // HK_INSPECTOR_PANEL_H
