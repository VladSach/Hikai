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

            node->visible = node->parent->visible;

            node->dirty = false;
            propagate = true;

            if (node->object) { dirty_.push(node); }
        }

        // FIX: probably temp (or not)
        if (node->object && node->visible && node->entity) {
            if (node->entity->dirty.any()) {
                dirty_.push(node);
            }
        }

        // Draw debug sphere around lights
        if (node->object && node->debug_draw && node->entity && node->entity->light) {
            hk::Light &light = *node->entity->light;

            hkm::vec4f color = node->entity->light->color;
            hk::dd::ShapeDesc desc = {};
            desc.color = {color.x, color.y, color.z};
            desc.thickness = 6.f;

            hkm::vec3f normal = hkm::vec3f(0, 0, 1) * node->world.rotation;

            switch(node->entity->light->type) {
            case Light::Type::POINT_LIGHT: {
                desc.thickness = 3.f;
                hk::dd::sphere(desc, node->world.pos, light.range);
            } break;
            case Light::Type::SPOT_LIGHT: {
                // hk::dd::circle({{c.x, c.y, c.z}, 3}, node->world.pos, .5f, normal);
                hk::dd::line(desc, node->world.pos, node->world.pos + normal * .2f);
                desc.thickness = 3.f;
                hk::dd::conical_frustum(
                        desc,
                        node->world.pos, light.inner_cutoff,
                        node->world.pos + normal * light.range, light.outer_cutoff);
            } break;
            case Light::Type::DIRECTIONAL_LIGHT: {
                hk::dd::line(desc, node->world.pos, node->world.pos + normal * .2f);
            } break;

            default: break;
            }
        }

        if (node->object && node->debug_draw && node->entity && node->entity->camera) {
            hk::Camera &camera = *node->entity->camera;

            // hk::dd::line({{1, 0, 0}, 5, false}, camera.position(), camera.position() + camera.top());
            // hk::dd::line({{0, 0, 1}, 5, false}, camera.position(), camera.position() + camera.right());
            // hk::dd::line({{0, 1, 0}, 5, false}, camera.position(), camera.position() + camera.forward());

            hk::dd::view_frustum({{0, 1, 0}, 3, true}, camera.viewProjectionInv());
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
    snode->entity = node.entity;
    snode->debug_draw = node.debug_draw;
    snode->visible = node.visible;

    if (snode->object && snode->entity && snode->entity->hndlMesh) {
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

    if (hkm::toEulerAngles(transform.rotation).length() < 0.01f) {
        switch (light.type) {
        case Light::Type::SPOT_LIGHT: {
            node->loaded.rotation =
                hkm::fromAxisAngle({1, 0, 0}, 90.f * hkm::degree2rad);
            node->local.rotation = node->loaded.rotation;
        } break;
        case Light::Type::DIRECTIONAL_LIGHT: {
            node->loaded.rotation =
                hkm::fromAxisAngle({1, 0, 0}, 45.f * hkm::degree2rad);
            node->local.rotation = node->loaded.rotation;
        } break;
        default: break;
        }
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
                object.create(mesh.mesh, mesh.name);

                node->entity->dirty.flip(0);
            }

            // TODO: i don't think that works right, if i clear position,
            // that means it can't be more that one instance?
            object.instances.clear();
            object.instances.push_back(node->world.toMat4f());

            if (node->entity->dirty.test(1)) {
                hk::MaterialAsset asset = hk::assets()->getMaterial(node->entity->hndlMaterial);
                object.rm.material = &asset.data;

                object.rm.build(
                    renderer.offscreen_.render_pass_,
                    sizeof(InstanceData),
                    renderer.global_desc_layout.handle(),
                    renderer.offscreen_.set_layout_.handle(),
                    renderer.offscreen_.formats_,
                    renderer.offscreen_.depth_format_,
                    asset.name);

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
        } else if (node->entity->camera) {
            hk::Camera &camera = *node->entity->camera;

            camera.setWorldOffset(node->world.pos);
            camera.setWorldRotation(node->world.rotation);
            camera.update();

            if (node->entity->dirty.test(3)) {
                node->entity->dirty.flip(3);
            }
        }
    }

    // FIX: temp
    LightSources sources;
    for (u32 i = 0; i < context.lights.size(); ++i) {
        auto light = context.lights.at(i);

        if (light.light->type == hk::Light::Type::POINT_LIGHT) {
            sources.point_lights[sources.point_count].color = light.light->color;
            sources.point_lights[sources.point_count].intensity = light.light->intensity;

            sources.point_lights[sources.point_count].pos = light.transform.pos;

            ++sources.point_count;
        } else if (light.light->type == hk::Light::Type::SPOT_LIGHT) {
            sources.spot_lights[sources.spot_count].color = light.light->color;
            sources.spot_lights[sources.spot_count].inner_cutoff = light.light->inner_cutoff;
            sources.spot_lights[sources.spot_count].outer_cutoff = light.light->outer_cutoff;

            sources.spot_lights[sources.spot_count].pos = light.transform.pos;
            sources.spot_lights[sources.spot_count].dir = hkm::toEulerAngles(light.transform.rotation);

            ++sources.spot_count;
        } else if (light.light->type == hk::Light::Type::DIRECTIONAL_LIGHT) {
            sources.directional_lights[sources.directinal_count].color = light.light->color;
            sources.directional_lights[sources.directinal_count].dir =
                hkm::toEulerAngles(light.transform.rotation);

            ++sources.directinal_count;
        }
    }
    renderer.updateLights(sources);
}

}
