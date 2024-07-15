#ifndef HK_BUFFER_H
#define HK_BUFFER_H

#include "defines.h"
#include "vendor/vulkan/vulkan.h"

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

public:
    Buffer() = default;
    Buffer(const VkDevice device,
           const VkPhysicalDevice physical,
           u32 count, u32 stride,
           Buffer::Type type)
    {
        init(device, physical, count, stride, type);
    }

    ~Buffer() { deinit(); }

    void init(const VkDevice device,
              const VkPhysicalDevice physical,
              u32 count, u32 stride,
              Buffer::Type type);
    void deinit();

    void update(const void *data,
                const VkCommandPool commandPool, const VkQueue queue);

    void map();
    void unmap();

    void write(const void *data);

private:
    void createBuffer(VkBuffer &buffer, VkDeviceMemory &bufferMemory,
                      VkBufferUsageFlags usage,
                      VkMemoryPropertyFlags properties);

public:

    constexpr Buffer::Type type() const { return type_; }
    constexpr VkDeviceSize size() const { return size_; }
    constexpr u32 stride() const { return stride_; }
    constexpr u32 count() const { return size_ / stride_; }

    const VkBuffer& buffer() const { return buffer_; }
    const VkDevice& device() const { return device_; }

private:
    Buffer::Type type_;

    VkBuffer buffer_ = VK_NULL_HANDLE;
    VkDeviceMemory memory_ = VK_NULL_HANDLE;

    VkDevice device_ = VK_NULL_HANDLE;
    VkPhysicalDevice physical_ = VK_NULL_HANDLE;

    VkDeviceSize size_ = 0;
    u32 stride_ = 0;

    void *mapped = nullptr;
};

void copyBuffer(Buffer srcBuffer, Buffer dstBuffer,
                VkCommandPool commandPool, VkQueue queue);

#endif // HK_BUFFER_H
