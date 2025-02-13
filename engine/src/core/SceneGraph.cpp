#include "SceneGraph.h"

namespace hk {

void SceneGraph::init()
{
    root_ = new SceneNode();
    root_->name = "World";
    root_->dirty = false;

    size_ = 0;
}

void SceneGraph::update()
{
    std::function<void(SceneNode*, b8)> traverseSceneGraph;
    traverseSceneGraph = [&](SceneNode *node, b8 propagate) {

        if (node->dirty || propagate) {
            Transform tr = (node->parent) ? node->parent->world : Transform();

            // FIX: Why the fuck it works the other way around?
            // should be parent->world * node->local
            node->world = node->local * tr;

            node->dirty = false;
            propagate = true;

            if (node->object) { dirty_.push(node); }
        }


        // FIX: probably temp
        else if (node->object) {
            if (node->entity) {
                if (node->entity->dirty) {
                    dirty_.push(node);
                }
            }
        }

        for (auto child : node->children) {
            traverseSceneGraph(child, propagate);
        }
    };

    traverseSceneGraph(root_, root_->dirty);
}

void SceneGraph::addNode(const SceneNode &node)
{
    SceneNode *snode = new SceneNode();
    SceneNode *parent = node.parent ? node.parent : root_;

    snode->idx = size_++;
    snode->name = node.name;
    snode->object = node.object;
    snode->dirty = true;
    snode->parent = parent;
    snode->loaded = node.loaded;
    snode->local = node.loaded;
    // snode->entity = node.entity;

    if (snode->object) {
        ++objects_;
        dirty_.push(snode);
    }

    parent->children.push_back(snode);
}

void SceneGraph::addModel(u32 handle, const Transform &transform)
{
    hk::ModelAsset model = hk::assets()->getModel(handle);

    SceneNode *parent = new SceneNode();
    parent->idx = size_++;
    parent->name = model.name;
    parent->object = false;
    parent->dirty = true;
    parent->handle = handle;
    parent->parent = root_;
    parent->loaded = transform;
    parent->local = parent->loaded;

    u32 hndlMesh = model.hndlRootMesh;
    hk::MeshAsset mesh = hk::assets()->getMesh(hndlMesh);

    std::function<void(SceneNode*, hk::MeshAsset*)> addMeshes;
    addMeshes = [&](SceneNode *parent, hk::MeshAsset* asset){
        for (auto &child : asset->children) {
            SceneNode *node = new SceneNode();
            node->idx = size_++;
            node->name = child->name;
            node->object = true;
            node->dirty = true;
            node->parent = parent;
            node->loaded = child->instances.at(0);
            node->local = node->loaded;

            Entity *entity = new Entity();
            entity->attachMesh(child->handle);
            if (child->hndlTextures.size())
                entity->attachMaterial(child->hndlTextures.at(0));
            node->entity = entity;

            node->idxObject = objects_;
            ++objects_;

            parent->children.push_back(node);

            addMeshes(node, child);
        }
    };

    addMeshes(parent, &mesh);

    root_->children.push_back(parent);
}

void SceneGraph::addLight(Light light, const Transform &transform)
{
    SceneNode *node = new SceneNode();
    node->idx = size_++;
    node->name = "Light TEST";
    node->object = true;
    node->dirty = true;
    node->parent = root_;
    node->loaded = transform;
    node->local = node->loaded;

    Entity *entity = new Entity();
    entity->attachLight(light);
    node->entity = entity;

    node->idxObject = lights_;
    ++lights_;

    root_->children.push_back(node);
}

void SceneGraph::updateDrawContext(DrawContext &context, Renderer &renderer)
{
    if (dirty_.empty()) { return; }

    vkDeviceWaitIdle(renderer.device_);

    context.objects.resize(objects_);
    context.lights.resize(lights_);

    for (; !dirty_.empty(); dirty_.pop()) {
        SceneNode *node = dirty_.front();
        RenderObject &object = context.objects.at(node->idxObject);

        // TODO: build only once
        if ((node->entity->dirty >> 7) & 1) {
            hk::MeshAsset mesh = hk::assets()->getMesh(node->entity->hndlMesh);
            object.create(mesh.mesh);

            node->entity->dirty ^= 0b10000000;
        }

        // TODO: i don't think that works right, if i clear position,
        // that means it can't be more that one instance?
        object.instances.clear();
        object.instances.push_back(node->world.toMat4f());

        if ((node->entity->dirty >> 6) & 1) {
            // object.materials.clear();
            object.materials.resize(1);

            object.materials.at(0).clear();

            object.materials.at(0).material =
                &hk::assets()->getMaterial(node->entity->hndlMaterial).data;

            object.materials.at(0).build(
                renderer.offscreen_.render_pass_,
                sizeof(ModelToWorld),
                renderer.sceneDescriptorLayout,
                renderer.swapchain_.format(),
                renderer.offscreen_.depth_.format());

            // object.materials2.resize(1);
            // object.materials2.at(0) = object.materials.at(0).write(renderer.frameDescriptors, renderer.samplerLinear);

            node->entity->dirty ^= 0b01000000;
        }
    }
}

}
