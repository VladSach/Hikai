#ifndef HK_MODEL_H
#define HK_MODEL_H

#include "Entity.h"
#include "Mesh.h"

#include "renderer/vkwrappers/Buffer.h"
#include "renderer/vkwrappers/Image.h"

#include "renderer/Material.h"

namespace hk {

class Model : public Entity {
public:
    ~Model() { deinit(); }

    void deinit()
    {
        indexBuffer_.deinit();
        vertexBuffer_.deinit();
    }

    void populateBuffers()
    {
        ranges_.clear();

        hk::vector<Vertex> vertices;
        hk::vector<u32> indices;

        u32 vn = 0;
        u32 inn = 0;
        u32 prev_vn = 0;
        u32 prev_inn = 0;
        for (auto &mesh : meshes_) {
        // for (auto &handle : meshes) {
            //Mesh mesh = hk::assets()->getMesh(handle);

            prev_vn += vn;
            prev_inn += inn;
            vn = 0;
            inn = 0;

            for (auto &v : mesh.vertices) {
                vertices.push_back(v);
                ++vn;
            }

            for (auto &triangle : mesh.triangles) {
                indices.push_back(triangle.indices[0]);
                indices.push_back(triangle.indices[1]);
                indices.push_back(triangle.indices[2]);
                inn += 3;
            }

            ranges_.push_back({prev_vn, prev_inn, vn, inn});
        }

        Buffer::BufferDesc vertexDesc = {};
        vertexDesc.type = Buffer::Type::VERTEX_BUFFER;
        vertexDesc.usage = Buffer::Usage::TRANSFER_DST;
        vertexDesc.property = Buffer::Property::GPU_LOCAL;
        vertexDesc.size = vertices.size();
        vertexDesc.stride = sizeof(Vertex);
        vertexBuffer_.init(vertexDesc);

        Buffer::BufferDesc indexDesc = {};
        indexDesc.type = Buffer::Type::INDEX_BUFFER;
        indexDesc.usage = Buffer::Usage::TRANSFER_DST;
        indexDesc.property = Buffer::Property::GPU_LOCAL;
        indexDesc.size = indices.size();
        indexDesc.stride = sizeof(indices[0]);
        indexBuffer_.init(indexDesc);

        vertexBuffer_.update(vertices.data());
        indexBuffer_.update(indices.data());

        vertexCnt = vertices.size();
        indexCnt = indices.size();
    }

    void bind(VkCommandBuffer commandBuffer) {
        vertexBuffer_.bind(commandBuffer);
        indexBuffer_.bind(commandBuffer);
    }

public:
    const u32 indexCount() const { return indexCnt; }

public:
    hk::vector<Mesh> meshes_;

    struct MeshRange {
        u32 vertexOffset; // offset in vertices
        u32 indexOffset;  // offset in indices
        u32 vertexNum;    // num of vertices
        u32 indexNum;     // num of indices
    };
    hk::vector<MeshRange> ranges_; // where each mesh data is stored in vertices

    Buffer vertexBuffer_; // stores vertices of all Meshes of this Model
    Buffer indexBuffer_;  // stores indices of all Meshes of this Model

    u32 hndlMaterial;
    MaterialInstance matInstance;

    u32 hndlRootMesh;

    // info
    u32 vertexCnt = 0;
    u32 indexCnt = 0;
};

}

#endif // HK_MODEL_H
