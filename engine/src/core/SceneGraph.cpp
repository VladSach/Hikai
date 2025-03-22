#include "SceneGraph.h"

#include "renderer/ui/debug_draw.h"

namespace hk {

void SceneGraph::init()
{
    root_ = new SceneNode();
    root_->name = "World";
    root_->dirty = false;

    size_ = 0;
}

void SceneGraph::deinit()
{
    LOG_DEBUG("Destroying Scene Graph");
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

        // FIX: probably temp (or not)
        else if (node->object) {
            if (node->entity) {
                if (node->entity->dirty.any()) {
                    dirty_.push(node);
                }
            }
        }

        // Draw debug sphere around lights
        if (node->object && node->entity && node->entity->light && node->debug_draw) {
            hk::Light &light = *node->entity->light;
            // FIX: temp
            hkm::vec4f c = node->entity->light->color;

            hkm::vec3f normal = normalize(hkm::toEulerAngles(node->world.rotation));

            switch(node->entity->light->type) {
            case Light::Type::POINT_LIGHT: {
                hk::dd::sphere({{c.x, c.y, c.z}, 3}, node->world.pos, light.range);
            } break;
            case Light::Type::SPOT_LIGHT: {
                // hk::dd::circle({{c.x, c.y, c.z}, 3}, node->world.pos, .5f, normal);
                hk::dd::conical_frustum(
                        {{c.x, c.y, c.z}, 3},
                        node->world.pos, light.inner_cutoff,
                        node->world.pos + normal * light.range, light.outer_cutoff);
                hk::dd::line({{c.x, c.y, c.z}, 6}, node->world.pos, node->world.pos + normal * .2f);
            } break;
            case Light::Type::DIRECTIONAL_LIGHT: {
                hk::dd::line({{c.x, c.y, c.z}, 6}, node->world.pos, node->world.pos + normal * .2f);
            } break;

            default: break;
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
            if (child->hndlTextures.size()) {
                entity->attachMaterial(child->hndlTextures.at(0));
            }
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

void SceneGraph::addLight(const Light &light, const Transform &transform)
{
    SceneNode *node = new SceneNode();
    node->idx = size_++;
    node->name = "Light TEST";
    node->object = true;
    node->dirty = true;
    node->parent = root_;
    node->loaded = transform;
    node->local = node->loaded;

    // FIX: maybe temp? i think lights should have default rotation
    if (hkm::toEulerAngles(transform.rotation).length() < 0.01f) {
        node->loaded.rotation = hkm::fromEulerAngles({0.f, -90.f, 0.f});
        node->local.rotation = node->loaded.rotation;
    }

    node->debug_draw = true;

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

    // FIX: why is this here again?
    vkDeviceWaitIdle(renderer.device_);

    context.objects.resize(objects_);
    context.lights.resize(lights_);

    for (; !dirty_.empty(); dirty_.pop()) {
        SceneNode *node = dirty_.front();

        // If node is a Mesh object
        if (node->entity->hndlMesh || node->entity->hndlMaterial) {
            RenderObject &object = context.objects.at(node->idxObject);

            // TODO: build only once
            if (node->entity->dirty.test(0)) {
                hk::MeshAsset mesh = hk::assets()->getMesh(node->entity->hndlMesh);
                object.create(mesh.mesh);

                node->entity->dirty.flip(0);
            }

            // TODO: i don't think that works right, if i clear position,
            // that means it can't be more that one instance?
            object.instances.clear();
            object.instances.push_back(node->world.toMat4f());

            if (node->entity->dirty.test(1)) {
                object.rm.material =
                    &hk::assets()->getMaterial(node->entity->hndlMaterial).data;

                object.rm.build(
                    renderer.offscreen_.render_pass_,
                    sizeof(InstanceData),
                    renderer.global_desc_layout.handle(),
                    renderer.offscreen_.set_layout_.handle(),
                    renderer.offscreen_.formats_,
                    renderer.offscreen_.depth_.format());

                object.material = object.rm.write(renderer.global_desc_alloc);

                node->entity->dirty.flip(1);
            }
        } else if (node->entity->light) {
            RenderLight &light = context.lights.at(node->idxObject);

            light.transform = node->world;

            if (node->entity->dirty.test(2)) {
                light.light = node->entity->light;

                node->entity->dirty.flip(2);
            }
        }
    }

    // FIX: temp
    LightSources lightsAA;
    for (u32 i = 0; i < context.lights.size(); ++i) {
        auto light = context.lights.at(i);

        if (light.light->type == hk::Light::Type::POINT_LIGHT) {
            lightsAA.pointlights[lightsAA.pointlightsSize].color = light.light->color;
            lightsAA.pointlights[lightsAA.pointlightsSize].intensity = light.light->intensity;

            lightsAA.pointlights[lightsAA.pointlightsSize].pos = light.transform.pos;

            ++lightsAA.pointlightsSize;
        } else if (light.light->type == hk::Light::Type::SPOT_LIGHT) {
            lightsAA.spotlights[lightsAA.spotlightsSize].color = light.light->color;
            lightsAA.spotlights[lightsAA.spotlightsSize].inner_cutoff = light.light->inner_cutoff;
            lightsAA.spotlights[lightsAA.spotlightsSize].outer_cutoff = light.light->outer_cutoff;

            lightsAA.spotlights[lightsAA.spotlightsSize].pos = light.transform.pos;
            lightsAA.spotlights[lightsAA.spotlightsSize].dir = hkm::toEulerAngles(light.transform.rotation);

            ++lightsAA.spotlightsSize;
        } else if (light.light->type == hk::Light::Type::DIRECTIONAL_LIGHT) {
            lightsAA.directional.color = light.light->color;
            lightsAA.directional.dir = hkm::toEulerAngles(light.transform.rotation);
        }
    }
    renderer.updateLights(lightsAA);
}

}
