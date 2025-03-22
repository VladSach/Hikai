#ifndef HK_DRAW_CONTEXT_H
#define HK_DRAW_CONTEXT_H

#include "object/Light.h"
#include "renderer/Material.h"
#include "renderer/vkwrappers/Buffer.h"

#include "renderer/object/Mesh.h"
#include "renderer/object/Transform.h"

namespace hk {

struct RenderObject {
    // Mesh
    Buffer vertexBuffer;
    Buffer indexBuffer;

    // Mesh instances || Mesh 1 <=> * Mesh Instances
    hk::vector<hkm::mat4f> instances;

    // Instance material || Mesh Instance 1 <=> 1 Material
    // FIX: different meshes can have the same material, it's not a 1 to 1
    // TODO: take out render material out of here(?)
    RenderMaterial rm;
    MaterialInstance material;

    ~RenderObject() { deinit(); }

    void deinit()
    {
        rm.clear();
        indexBuffer.deinit();
        vertexBuffer.deinit();
    }

    void create(const hk::Mesh &mesh)
    {
        if (vertexBuffer.buffer() || indexBuffer.buffer()) { return; }

        Buffer::BufferDesc vertexDesc = {};
        vertexDesc.type = Buffer::Type::VERTEX_BUFFER;
        vertexDesc.usage = Buffer::Usage::TRANSFER_DST;
        vertexDesc.property = Buffer::Property::GPU_LOCAL;
        vertexDesc.size = mesh.vertices.size();
        vertexDesc.stride = sizeof(Vertex);
        vertexBuffer.init(vertexDesc);

        Buffer::BufferDesc indexDesc = {};
        indexDesc.type = Buffer::Type::INDEX_BUFFER;
        indexDesc.usage = Buffer::Usage::TRANSFER_DST;
        indexDesc.property = Buffer::Property::GPU_LOCAL;
        indexDesc.size = mesh.indices.size();
        indexDesc.stride = sizeof(mesh.indices.at(0));
        indexBuffer.init(indexDesc);

        vertexBuffer.update(mesh.vertices.data());
        indexBuffer.update(mesh.indices.data());
    }

    void bind(VkCommandBuffer cmd)
    {
        vertexBuffer.bind(cmd);
        indexBuffer.bind(cmd);
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
