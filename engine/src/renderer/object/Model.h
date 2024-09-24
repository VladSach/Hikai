#ifndef HK_MODEL_H
#define HK_MODEL_H

#include "renderer/object/Vertex.h"
#include "renderer/vkwrappers/Buffer.h"

#include "utils/containers/hkvector.h"

namespace hk {

class Model {
public:
    void init(const hk::vector<Vertex> &vertices,
              const hk::vector<u32> &indices)
    {
        vertices_ = vertices;
        indices_ = indices;
    }

    void deinit()
    {
        indexBuffer_.deinit();
        vertexBuffer_.deinit();
    }

    void populateBuffers()
    {
        Buffer::BufferDesc vertexDesc = {};
        vertexDesc.type = Buffer::Type::VERTEX_BUFFER;
        vertexDesc.usage = Buffer::Usage::TRANSFER_DST;
        vertexDesc.property = Buffer::Property::GPU_LOCAL;
        vertexDesc.size = vertices_.size();
        vertexDesc.stride = sizeof(Vertex);
        vertexBuffer_.init(vertexDesc);

        Buffer::BufferDesc indexDesc = {};
        indexDesc.type = Buffer::Type::INDEX_BUFFER;
        indexDesc.usage = Buffer::Usage::TRANSFER_DST;
        indexDesc.property = Buffer::Property::GPU_LOCAL;
        indexDesc.size = indices_.size();
        indexDesc.stride = sizeof(indices_[0]);
        indexBuffer_.init(indexDesc);

        vertexBuffer_.update(vertices_.data());
        indexBuffer_.update(indices_.data());
    }

    void bind(VkCommandBuffer commandBuffer) {
        vertexBuffer_.bind(commandBuffer);
        indexBuffer_.bind(commandBuffer);
    }

public:
    const u32 indexCount() const { return indices_.size(); }

private:
    Buffer vertexBuffer_;
    Buffer indexBuffer_;

    hk::vector<Vertex> vertices_;
    hk::vector<u32> indices_;
};

}

#endif // HK_MODEL_H
