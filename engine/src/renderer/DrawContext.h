#ifndef HK_DRAW_CONTEXT_H
#define HK_DRAW_CONTEXT_H

#include "object/Light.h"
#include "renderer/Material.h"
#include "renderer/vkwrappers/Buffer.h"

#include "renderer/object/Mesh.h"

namespace hk {

struct RenderObject {
    // Mesh
    Buffer vertexBuffer;
    Buffer indexBuffer;

    // Mesh instances || Mesh 1 <=> * Mesh Instances
    hk::vector<hkm::mat4f> instances;

    // Instance material || Mesh Instance 1 <=> 1 Material
    // TODO: Would want to keep here material instance and not render material
    // but can't figure out descriptor set lifetime right now
    hk::vector<RenderMaterial> materials;
    // hk::vector<MaterialInstance> materials2;

    ~RenderObject() { deinit(); }

    void deinit()
    {
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

    void bind(VkCommandBuffer commandBuffer) {
        vertexBuffer.bind(commandBuffer);
        indexBuffer.bind(commandBuffer);
    }
};

struct RenderLight {
    Light light;
    hkm::mat4f pos;
};

struct DrawContext {
    hk::vector<RenderObject> objects;
    hk::vector<RenderLight> lights;
};

}

#endif // HK_DRAW_CONTEXT_H
