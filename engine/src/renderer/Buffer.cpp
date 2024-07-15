#include "Buffer.h"

void Buffer::init(const VkDevice device,
                  const VkPhysicalDevice physical,
                  u32 count, u32 stride,
                  Buffer::Type type)
{
    if (buffer_ != VK_NULL_HANDLE) {
        LOG_WARN("Buffer is already initialized");
        return;
    }

    device_ = device;
    physical_ = physical;
    type_ = type;

    ALWAYS_ASSERT(static_cast<u32>(type_), "Buffer Type can not be NONE");
    ALWAYS_ASSERT(device_ != VK_NULL_HANDLE,
                  "Can not create buffer without logical device");
    ALWAYS_ASSERT(physical_ != VK_NULL_HANDLE,
                  "Can not create buffer without physical device");

    stride_ = stride;
    size_ =  stride_ * count;

    u32 usage;
    u32 properties;
    switch(type_) {
    case Buffer::Type::VERTEX_BUFFER: {
        usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        // properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    } break;

    case Buffer::Type::INDEX_BUFFER: {
        usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        // properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    } break;

    case Buffer::Type::UNIFORM_BUFFER: {
        usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    } break;

    case Buffer::Type::STAGING_BUFFER: {
        usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    } break;

    default:
        usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    }

    createBuffer(buffer_, memory_,
                 static_cast<VkBufferUsageFlags>(usage),
                 static_cast<VkMemoryPropertyFlagBits>(properties));
}

void Buffer::deinit()
{
    if (buffer_ != VK_NULL_HANDLE)
        vkDestroyBuffer(device_, buffer_, nullptr);
    if (memory_ != VK_NULL_HANDLE)
        vkFreeMemory(device_, memory_, nullptr);
}

void Buffer::update(const void *data,
                    const VkCommandPool commandPool, const VkQueue queue)
{
    // TODO: fix staging buffers
    // Buffer stagingBuffer;
    // stagingBuffer.init(device_, physical_,
    //                    count(), stride_,
    //                    Type::STAGING_BUFFER);
    //
    // stagingBuffer.map();
    //     stagingBuffer.write(data);
    // stagingBuffer.unmap();
    //
    // copyBuffer(stagingBuffer, *this, commandPool, queue);
    //
    // stagingBuffer.deinit();
    //
    map();
    write(data);
    unmap();
}

void Buffer::map()
{
    ALWAYS_ASSERT(!mapped, "Memory already mapped");
    vkMapMemory(device_, memory_, 0, size_, 0, &mapped);
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
    memcpy(mapped, data, size_);
}

void Buffer::createBuffer(VkBuffer &buffer, VkDeviceMemory &bufferMemory,
                          VkBufferUsageFlags usage,
                          VkMemoryPropertyFlags properties)
{
    VkResult err;

    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size_;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    err = vkCreateBuffer(device_, &bufferInfo, nullptr, &buffer);
    ALWAYS_ASSERT(!err, "Failed to create Vulkan Buffer");

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device_, buffer, &memRequirements);

    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physical_, &memProperties);

    u32 memIndex = 0;
    for (; memIndex < memProperties.memoryTypeCount; memIndex++) {
        if ((memRequirements.memoryTypeBits & (1 << memIndex)) &&
            (memProperties.memoryTypes[memIndex].propertyFlags & properties)
            == properties)
        {
            break;
        }
    }

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = memIndex;

    err = vkAllocateMemory(device_, &allocInfo, nullptr, &bufferMemory);
    ALWAYS_ASSERT(!err, "Failed to allocate Vulkan Buffer Memory");

    vkBindBufferMemory(device_, buffer, bufferMemory, 0);
}

void copyBuffer(Buffer srcBuffer, Buffer dstBuffer,
                VkCommandPool commandPool, VkQueue queue)
{
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer tmpBuffer;
    vkAllocateCommandBuffers(srcBuffer.device(), &allocInfo, &tmpBuffer);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(tmpBuffer, &beginInfo);
    {
        VkBufferCopy copyRegion = {};
        copyRegion.size = srcBuffer.size();
        vkCmdCopyBuffer(tmpBuffer,
                        srcBuffer.buffer(),
                        dstBuffer.buffer(),
                        1, &copyRegion);
    }
    vkEndCommandBuffer(tmpBuffer);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &tmpBuffer;

    vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);

    vkFreeCommandBuffers(srcBuffer.device(), commandPool, 1, &tmpBuffer);
}
