#ifndef HK_DRAW_CONTEXT_H
#define HK_DRAW_CONTEXT_H

#include "object/Light.h"
#include "renderer/Material.h"

#include "resources.h"

#include "renderer/object/Mesh.h"
#include "renderer/object/Transform.h"

namespace hk {

struct RenderObject {
    // Mesh
    BufferHandle vertex;
    BufferHandle index;

    // Mesh instances || Mesh 1 <=> * Mesh Instances
    hk::vector<hkm::mat4f> instances;

    // Instance material || Mesh Instance 1 <=> 1 Material
    // FIX: different meshes can have the same material, it's not a 1 to 1
    // TODO: take out render material out of here(?)
    RenderMaterial rm;
    MaterialInstance material;

    // ~RenderObject() { deinit(); }

    void deinit()
    {
        rm.clear();
        bkr::destroy_buffer(vertex);
        bkr::destroy_buffer(index);
    }

    void create(const hk::Mesh &mesh, const std::string &name)
    {
        BufferDesc vertex_desc = {};
        vertex_desc.type = BufferType::VERTEX_BUFFER;
        vertex_desc.access = MemoryType::GPU_LOCAL;
        vertex_desc.size = mesh.vertices.size();
        vertex_desc.stride = sizeof(Vertex);
        vertex = bkr::create_buffer(vertex_desc, name);

        BufferDesc index_desc = {};
        index_desc.type = BufferType::INDEX_BUFFER;
        index_desc.access = MemoryType::GPU_LOCAL;
        index_desc.size = mesh.indices.size();
        index_desc.stride = sizeof(mesh.indices.at(0));
        index = bkr::create_buffer(index_desc, name);

        bkr::update_buffer(vertex, mesh.vertices.data());
        bkr::update_buffer(index, mesh.indices.data());
    }

    void bind(VkCommandBuffer cmd)
    {
        bkr::bind_buffer(vertex, cmd);
        bkr::bind_buffer(index, cmd);
    }
};

struct RenderLight {
    Light *light;
    Transform transform;
};

struct DrawContext {
    hk::vector<RenderObject> objects;
    hk::vector<RenderLight> lights;
};

}

#endif // HK_DRAW_CONTEXT_H
