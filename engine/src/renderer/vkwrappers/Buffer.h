#ifndef HK_BUFFER_H
#define HK_BUFFER_H

#include "vendor/vulkan/vulkan.h"

#include "defines.h"

#include "utils/numerics/hkbitflag.h"

namespace hk {

class Buffer {
public:
    enum class Type {
        NONE = 0,

        VERTEX_BUFFER,
        INDEX_BUFFER,
        UNIFORM_BUFFER,
        STAGING_BUFFER,

        MAX_BUFFER_TYPE
    };

    enum class Usage {
        NONE = 0,

        TRANSFER_SRC,
        TRANSFER_DST,

        MAX_BUFFER_USAGE
    };

    enum class Property {
        NONE = 0,

        GPU_LOCAL     = 1 << 1,
        CPU_ACESSIBLE = 1 << 2,
        CPU_COHERENT  = 1 << 3,
        CPU_CACHED    = 1 << 4,
    };

    struct BufferDesc {
        Buffer::Type type;
        Buffer::Usage usage;
        hk::bitflag<Property> property;

        u32 size;
        u32 stride;
    };

    struct VulkanBufferDesc {
        VkDeviceSize size;
        VkBufferUsageFlags usage;
        VkMemoryPropertyFlags properties;
    };

public:
    Buffer() = default;
    ~Buffer() { deinit(); }

    void init(const BufferDesc &desc);
    void deinit();

    void map();
    void unmap();

    void write(const void *data);
    void update(const void *data);

    void bind(VkCommandBuffer commandBuffer);

public:
    constexpr u32 size() const { return size_; }
    constexpr u32 stride() const { return stride_; }
    constexpr VkDeviceSize memsize() const { return size_ * stride_; }

    VkBuffer buffer() { return buffer_; }
    const VkBuffer& buffer() const { return buffer_; }

    VulkanBufferDesc getDeviceBufferDesc() const;

private:
    void allocateBuffer(const VulkanBufferDesc &desc);

private:
    Type type_ = Type::NONE;
    Usage usage_ = Usage::NONE;
    hk::bitflag<Property> property_ = Property::NONE;

    u32 size_ = 0;
    u32 stride_ = 0;

    void *mapped = nullptr;

    VkBuffer buffer_ = VK_NULL_HANDLE;
    VkDeviceMemory memory_ = VK_NULL_HANDLE;

    VkDevice device_ = VK_NULL_HANDLE;
};

REGISTER_ENUM(Buffer::Property);

}

#endif // HK_BUFFER_H
