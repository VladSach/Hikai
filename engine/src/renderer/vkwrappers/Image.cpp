#include "Image.h"

// FIX: temp
#include "renderer/UBManager.h"
#include "renderer/Descriptors.h"
#include "renderer/vkwrappers/Buffer.h"

#include "renderer/VulkanContext.h"

namespace hk {

void Image::init(const ImageDesc &desc)
{
    width_ = desc.width;
    height_ = desc.height;
    channels_ = desc.channels;

    usage_ = desc.usage;
    format_ = desc.format;
    layout_ = desc.layout;
    aspectMask_ = desc.aspectMask;

    VkImageUsageFlags flags = 0;
    if (usage_ & Usage::TRANSFER_SRC)
        flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    if (usage_ & Usage::TRANSFER_DST)
        flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    if (usage_ & Usage::SAMPLED)
        flags |= VK_IMAGE_USAGE_SAMPLED_BIT;
    if (usage_ & Usage::DEPTH_STENCIL_ATTACHMENT)
        flags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    VulkanImageDesc imageDesc = {
        width_, height_,
        format_,
        VK_IMAGE_TILING_OPTIMAL,
        flags,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    };

    allocateImage(imageDesc);

    // FIX: temp fix, later make all transitions possible
    VkImageLayout tmpLayout = layout_;
    layout_ = (layout_ == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) ?
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL : layout_;

    transitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, layout_);

    layout_ = tmpLayout;

    VkDevice device = hk::context()->device();

    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image_;
    viewInfo.format = format_;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.subresourceRange.aspectMask = aspectMask_;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkResult err = vkCreateImageView(device, &viewInfo, nullptr, &view_);
    ALWAYS_ASSERT(!err, "Failed to create Vulkan Image View");
}

void Image::deinit()
{
    width_ = 0;
    height_ = 0;
    channels_ = 0;

    usage_ = Usage::NONE;
    format_ = VK_FORMAT_UNDEFINED;
    layout_ = VK_IMAGE_LAYOUT_UNDEFINED;
    aspectMask_ = VK_IMAGE_ASPECT_NONE;

    VkDevice device = hk::context()->device();

    if (sampler_)
        vkDestroySampler(device, sampler_, nullptr);
    if (view_)
        vkDestroyImageView(device, view_, nullptr);

    if (image_)
        vkDestroyImage(device, image_, nullptr);
    if (memory_)
        vkFreeMemory(device, memory_, nullptr);
}

void Image::write(const void *pixels)
{
    u32 size = width_ * height_ * 4;

    Buffer::BufferDesc bufferDesc = {
        Buffer::Type::STAGING_BUFFER,
        Buffer::Usage::TRANSFER_SRC,
        Buffer::Property::CPU_ACESSIBLE | Buffer::Property::CPU_COHERENT,
        // FIX: techincally stride should be 0, but as it used only in mul,
        // for getting memsize - it works as if it would be 0
        size, 1
    };

    Buffer stagingBuffer;
    stagingBuffer.init(bufferDesc);

    stagingBuffer.update(pixels);

    // FIX: temp fix
    // transitionImageLayout(
    //     layout_,
    //     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);


    // Copy from buffer to image
    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = { width_, height_, 1 };

    hk::context()->submitImmCmd([&](VkCommandBuffer cmd) {
        vkCmdCopyBufferToImage(
            cmd,
            stagingBuffer.buffer(),
            image_,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &region);
    });

    transitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, layout_);

    // stagingBuffer.deinit();
}

void Image::bind()
{
    VkResult err;
    VkDevice device = hk::context()->device();

    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

    VkPhysicalDeviceProperties properties = {};
    vkGetPhysicalDeviceProperties(hk::context()->physical(), &properties);

    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    err = vkCreateSampler(device, &samplerInfo, nullptr, &sampler_);

    // FIX: temp
    hk::DescriptorWriter *writer = getGlobalDescriptorWriter();
    writer->writeImage(1, view_, sampler_, layout_,
                       VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
}

void Image::allocateImage(const VulkanImageDesc &desc)
{
    VkResult err;

    VkDevice device = hk::context()->device();
    VkPhysicalDevice physical = hk::context()->physical();

    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = desc.width;
    imageInfo.extent.height = desc.height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = desc.format;
    imageInfo.tiling = desc.tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = desc.usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    err = vkCreateImage(device, &imageInfo, nullptr, &image_);

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, image_, &memRequirements);

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

    err = vkAllocateMemory(device, &allocInfo, nullptr, &memory_);

    vkBindImageMemory(device, image_, memory_, 0);
}

void Image::transitionImageLayout(VkImageLayout oldLayout,
                                  VkImageLayout newLayout)
{
    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image_;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
        newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
               newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
               newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask =
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    } else {
        LOG_ERROR("Unsupported layout transition");
    }

    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

        if (format_ == VK_FORMAT_D32_SFLOAT_S8_UINT ||
            format_ == VK_FORMAT_D24_UNORM_S8_UINT)
        {
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    } else {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    hk::context()->submitImmCmd([&](VkCommandBuffer cmd) {
        vkCmdPipelineBarrier(
            cmd,
            sourceStage, destinationStage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );
    });
}

}
