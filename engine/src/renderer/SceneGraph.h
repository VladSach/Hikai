#ifndef HK_SCENE_GRAPH_H
#define HK_SCENE_GRAPH_H

#include "renderer/object/Entity.h"
#include "renderer/object/Model.h"

namespace hk {

struct SceneNode {
    SceneNode* parent = nullptr;
    hk::vector<SceneNode*> children;

    b8 dirty = false;

    u32 handle;

    std::string name;
};

class SceneGraph {
public:
    void init()
    {
        root = new SceneNode();
        root->name = "World";
    }

    void addModel(const Model& model)
    // void addModel(const u32 handle)
    {
        // model get handle = set to parent
        // SceneNode root;
        // for (auto &mesh : model.meshes_) {
        //     // Get mesh handles = set to child
        //     // How to set them based on hierarchy?
        // }

    }

private:
    SceneNode *root;
};

}

#endif // HK_SCENE_GRAPH_H
