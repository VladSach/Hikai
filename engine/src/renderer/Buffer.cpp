#include "Buffer.h"

void Buffer::init(const BufferDesc &desc)
{
    if (buffer_ != VK_NULL_HANDLE) {
        LOG_WARN("Buffer is already initialized");
        return;
    }

    type_ = desc.type;
    usage_ = desc.usage;
    property_ = desc.property;

    size_ = desc.size;
    stride_ = desc.stride;

    RenderDevice::BufferDesc deviceBufferDesc = getDeviceBufferDesc();
    RenderDevice::DeviceBuffer deviceBuffer =
        hk::device()->createBuffer(deviceBufferDesc);

    buffer_ = deviceBuffer.buffer;
    memory_ = deviceBuffer.bufferMemory;

    device_ = hk::device()->logical();
}

void Buffer::deinit()
{
    if (mapped) {
        LOG_WARN("Buffer should be unmapped before deinitialization");
        return;
    }

    if (buffer_ != VK_NULL_HANDLE)
        vkDestroyBuffer(device_, buffer_, nullptr);
    if (memory_ != VK_NULL_HANDLE)
        vkFreeMemory(device_, memory_, nullptr);

    buffer_ = VK_NULL_HANDLE;
    memory_ = VK_NULL_HANDLE;
    device_ = VK_NULL_HANDLE;

    type_ = Type::NONE;
    usage_ = Usage::NONE;
    property_ = Property::NONE;

    size_ = 0;
    stride_ = 0;

    mapped = nullptr;
}

void Buffer::update(const void *data)
{
    if (property_ & Property::CPU_ACESSIBLE) {
        map();
            write(data);
        unmap();

        return;
    }

    BufferDesc desc = {
        Type::STAGING_BUFFER,
        Usage::TRANSFER_SRC,
        Property::CPU_ACESSIBLE | Property::CPU_COHERENT,
        size_,
        stride_
    };

    Buffer stagingBuffer;
    stagingBuffer.init(desc);

    stagingBuffer.map();
        stagingBuffer.write(data);
    stagingBuffer.unmap();

    hk::device()->copyBuffer(stagingBuffer.buffer(), buffer_, memsize());

    // stagingBuffer.deinit();
}

void Buffer::map()
{
    ALWAYS_ASSERT(!mapped, "Memory already mapped");
    vkMapMemory(device_, memory_, 0, memsize(), 0, &mapped);
}

void Buffer::unmap()
{
    ALWAYS_ASSERT(mapped, "Memory should be mapped");
    vkUnmapMemory(device_, memory_);
    mapped = nullptr;
}

void Buffer::write(const void *data)
{
    ALWAYS_ASSERT(mapped,
                  "Memory should be mapped before writing to the buffer");
    memcpy(mapped, data, memsize());
}

void Buffer::bind(VkCommandBuffer commandBuffer)
{
    switch(type_) {
    case Type::VERTEX_BUFFER: {
        VkBuffer vertexBuffers[] = { buffer_ };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    } break;

    case Type::INDEX_BUFFER: {
        vkCmdBindIndexBuffer(commandBuffer, buffer_, 0, VK_INDEX_TYPE_UINT32);
    } break;

    default: return;

    }
}

RenderDevice::BufferDesc Buffer::getDeviceBufferDesc() const
{
    ALWAYS_ASSERT(
        static_cast<u32>(type_) &&
        static_cast<u32>(property_),
        "Buffer should be initialized"
    );

    RenderDevice::BufferDesc desc = {};
    desc.size = memsize();

    switch(type_) {
    case Buffer::Type::VERTEX_BUFFER: {
        desc.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    } break;

    case Buffer::Type::INDEX_BUFFER: {
        desc.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    } break;

    case Buffer::Type::UNIFORM_BUFFER: {
        desc.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    } break;

    case Buffer::Type::STAGING_BUFFER: break;

    default:
        LOG_ERROR("Unsupported Buffer Type:", static_cast<u32>(type_));
    }

    switch(usage_) {
    case Usage::TRANSFER_SRC: {
        desc.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    } break;

    case Usage::TRANSFER_DST: {
        desc.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    } break;

    case Usage::NONE: break;

    default:
        LOG_ERROR("Unsupported Buffer Usage:", static_cast<u32>(usage_));
    }

    if (property_ & Property::GPU_LOCAL)
        desc.properties |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    if (property_ & Property::CPU_ACESSIBLE)
        desc.properties |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    if (property_ & Property::CPU_COHERENT)
        desc.properties |= VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    if (property_ & Property::CPU_CACHED)
        desc.properties |= VK_MEMORY_PROPERTY_HOST_CACHED_BIT;

    // LOG_ERROR("Unsupported Buffer Property");

    return desc;
}
