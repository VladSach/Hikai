#include "Buffer.h"

#include "renderer/VulkanContext.h"

namespace hk {

static constexpr u64 align_size(u64 size, u64 alignment)
{
    // https://github.com/SaschaWillems/Vulkan/blob/master/examples/dynamicuniformbuffer/dynamicuniformbuffer.cpp#L297
    return (alignment == 0) ? size : (size + alignment - 1) & ~(alignment - 1);
}

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

    auto limits = hk::context()->physicalInfo().properties.limits;

    stride_ = (type_ != Type::UNIFORM_BUFFER) ?
        desc.stride : align_size(desc.stride, limits.minUniformBufferOffsetAlignment);

    device_ = hk::context()->device();

    VulkanBufferDesc allocDesc = getDeviceBufferDesc();
    allocateBuffer(allocDesc);
}

void Buffer::deinit()
{
    if (mapped) {
        LOG_WARN("Buffer should be unmapped before deinitialization");
        return;
    }

    // TODO: change to 'b8 in_use'
    if (device_) { vkDeviceWaitIdle(device_); }

    if (buffer_ != VK_NULL_HANDLE) {
        vkDestroyBuffer(device_, buffer_, nullptr);
        buffer_ = VK_NULL_HANDLE;
    }

    if (memory_ != VK_NULL_HANDLE) {
        vkFreeMemory(device_, memory_, nullptr);
        memory_ = VK_NULL_HANDLE;
    }

    device_ = VK_NULL_HANDLE;

    type_ = Type::NONE;
    usage_ = Usage::NONE;
    property_ = Property::NONE;

    size_ = 0;
    stride_ = 0;
}

void Buffer::resize(u32 size)
{
    BufferDesc desc = {
        type_, usage_, property_, size, stride_
    };

    deinit();
    init(desc);
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

    hk::context()->submitImmCmd([&](VkCommandBuffer cmd) {
        VkBufferCopy copyRegion = {};
        copyRegion.size = memsize();
        vkCmdCopyBuffer(cmd, stagingBuffer.buffer(), buffer_, 1, &copyRegion);
    });

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

Buffer::VulkanBufferDesc Buffer::getDeviceBufferDesc() const
{
    ALWAYS_ASSERT(static_cast<u32>(type_) && property_,
                  "Buffer should be initialized");

    Buffer::VulkanBufferDesc desc = {};
    desc.mem_size = memsize();

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

    return desc;
}

void Buffer::allocateBuffer(const VulkanBufferDesc &desc)
{
    VkResult err;

    VkBufferCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    info.size = desc.mem_size;
    info.usage = desc.usage;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    err = vkCreateBuffer(device_, &info, nullptr, &buffer_);
    ALWAYS_ASSERT(!err, "Failed to create Vulkan Buffer");

    VkMemoryRequirements mem_requirements;
    vkGetBufferMemoryRequirements(device_, buffer_, &mem_requirements);

    auto mem_properties = hk::context()->physicalInfo().memProperties;

    u32 mem_index = 0;
    for (; mem_index < mem_properties.memoryTypeCount; ++mem_index) {
        if ((mem_requirements.memoryTypeBits & (1 << mem_index)) &&
            (mem_properties.memoryTypes[mem_index].propertyFlags &
            desc.properties) == desc.properties)
        {
            break;
        }
    }

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = mem_requirements.size;
    allocInfo.memoryTypeIndex = mem_index;

    err = vkAllocateMemory(device_, &allocInfo, nullptr, &memory_);
    ALWAYS_ASSERT(!err, "Failed to allocate Vulkan Buffer Memory");

    vkBindBufferMemory(device_, buffer_, memory_, 0);
}

}
