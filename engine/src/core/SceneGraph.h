#ifndef HK_SCENE_GRAPH_H
#define HK_SCENE_GRAPH_H

#include "renderer/object/Entity.h"
#include "renderer/object/Transform.h"

#include "resources/AssetManager.h"

#include "renderer/DrawContext.h"

#include "renderer/Renderer.h"

#include <queue>

namespace hk {

struct SceneNode {
    u32 idx;

    SceneNode* parent = nullptr;
    hk::vector<SceneNode*> children;

    std::string name;

    Transform local;
    Transform world;
    Transform loaded;

    // SceneNode can be either object (Mesh, Light, Camera, etc)
    // or just collection of other nodes
    b8 object = false;
    // FIX: add enum what object type is it (than why need entity component?)

    // Viable only when node is an object
    Entity *entity = nullptr;

    // Change propagates to all children
    b8 dirty = false;

    // FIX: Unsure about this
    // only needed when node is a Model object
    u32 handle = 0;

    u32 idxObject = 0;

    b8 debug_draw = false;
    b8 visible = true;
};

class SceneGraph {
public:
    void init();
    void deinit();

    void update();

    HKAPI void addNode(const SceneNode &node);
    HKAPI void addModel(u32 handle, const Transform &transform = Transform());
    HKAPI void addLight(const Light &light, const Transform &transform = Transform());

    void updateDrawContext(DrawContext &context, Renderer &renderer);

public:
    SceneNode *root() { return root_; }
    constexpr u32 size() const { return size_; }
    constexpr u32 objects() const { return objects_; }

private:
    SceneNode *root_ = nullptr;
    u32 size_ = 0;

    u32 objects_ = 0; // Amount of objects in a scene
    u32 lights_ = 0;  // Amount of lights in a scene

    // Queue with nodes that requires change in draw context
    std::queue<SceneNode*> dirty_;
};

}

#endif // HK_SCENE_GRAPH_H
