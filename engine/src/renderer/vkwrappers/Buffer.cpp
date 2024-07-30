#include "Buffer.h"

#include "renderer/VulkanContext.h"

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

    // hk::device()->copyBuffer(stagingBuffer.buffer(), buffer_, memsize());
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
    ALWAYS_ASSERT(
        static_cast<u32>(type_) &&
        static_cast<u32>(property_),
        "Buffer should be initialized"
    );

    Buffer::VulkanBufferDesc desc = {};
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

void Buffer::allocateBuffer(const VulkanBufferDesc &desc)
{
    VkResult err;

    VkPhysicalDevice physical = hk::context()->physical(); 

    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = desc.size;
    bufferInfo.usage = desc.usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    err = vkCreateBuffer(device_, &bufferInfo, nullptr, &buffer_);
    ALWAYS_ASSERT(!err, "Failed to create Vulkan Buffer");

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device_, buffer_, &memRequirements);

    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physical, &memProperties);

    u32 memIndex = 0;
    for (; memIndex < memProperties.memoryTypeCount; ++memIndex) {
        if ((memRequirements.memoryTypeBits & (1 << memIndex)) &&
            (memProperties.memoryTypes[memIndex].propertyFlags &
            desc.properties) == desc.properties)
        {
            break;
        }
    }

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = memIndex;

    err = vkAllocateMemory(device_, &allocInfo, nullptr, &memory_);
    ALWAYS_ASSERT(!err, "Failed to allocate Vulkan Buffer Memory");

    vkBindBufferMemory(device_, buffer_, memory_, 0);
}
