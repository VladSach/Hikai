#include "Image.h"

#include "renderer/VulkanContext.h"
#include "renderer/vkwrappers/Buffer.h"

namespace hk {

void Image::init(const ImageDesc &desc)
{
    width_ = desc.width;
    height_ = desc.height;
    channels_ = desc.channels;

    usage_ = desc.usage;
    format_ = desc.format;
    aspect_mask_ = desc.aspect_mask;

    VkImageUsageFlags flags = 0;
    if (usage_ & Usage::TRANSFER_SRC)
        flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    if (usage_ & Usage::TRANSFER_DST)
        flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    if (usage_ & Usage::SAMPLED)
        flags |= VK_IMAGE_USAGE_SAMPLED_BIT;
    if (usage_ & Usage::COLOR_ATTACHMENT)
        flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
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

    transitionLayout(desc.layout);
}

void Image::deinit()
{
    width_ = 0;
    height_ = 0;
    channels_ = 0;

    mip_levels_ = 1;
    array_layers_ = 1;

    usage_ = Usage::NONE;
    format_ = VK_FORMAT_UNDEFINED;
    layout_ = VK_IMAGE_LAYOUT_UNDEFINED;
    aspect_mask_ = VK_IMAGE_ASPECT_NONE;

    VkDevice device = hk::context()->device();

    if (view_)
        vkDestroyImageView(device, view_, nullptr);

    if (image_)
        vkDestroyImage(device, image_, nullptr);
    if (memory_)
        vkFreeMemory(device, memory_, nullptr);
}

void Image::write(const void *pixels)
{
    u32 size = width_ * height_ * 4; // Assuming 4 bytes per pixel

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

    VkImageLayout old_layout = layout_;

    transitionLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    // Copy from buffer to image
    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    // region.bufferRowLength = 0;
    // region.bufferImageHeight = 0;
    region.bufferRowLength = width_;
    region.bufferImageHeight = height_;
    region.imageSubresource.aspectMask = aspect_mask_;
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

    transitionLayout(old_layout);

    stagingBuffer.deinit();
}

void Image::copy(Image &src)
{
    VkImageLayout src_old_layout = src.layout();
    src.transitionLayout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

    VkImageLayout dst_old_layout = layout_;
    transitionLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    VkImageCopy copyRegion = {};
    copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.srcSubresource.mipLevel = 0;
    copyRegion.srcSubresource.baseArrayLayer = 0;
    copyRegion.srcSubresource.layerCount = 1;
    copyRegion.dstSubresource.aspectMask = aspect_mask_;
    copyRegion.dstSubresource.mipLevel = 0;
    copyRegion.dstSubresource.baseArrayLayer = 0;
    copyRegion.dstSubresource.layerCount = 1;
    copyRegion.extent.width = width_;
    copyRegion.extent.height = height_;
    copyRegion.extent.depth = 1;

    hk::context()->submitImmCmd([&](VkCommandBuffer cmd) {
        vkCmdCopyImage(
            cmd,
            src.image_,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            image_,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &copyRegion
        );
    });

    src.transitionLayout(src_old_layout);

    transitionLayout(dst_old_layout);
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
    ALWAYS_ASSERT(!err, "Failed to create Vulkan Image");

    VkMemoryRequirements mem_requirements;
    vkGetImageMemoryRequirements(device, image_, &mem_requirements);

    VkPhysicalDeviceMemoryProperties mem_properties;
    vkGetPhysicalDeviceMemoryProperties(physical, &mem_properties);

    u32 mem_index = 0;
    for (; mem_index < mem_properties.memoryTypeCount; ++mem_index) {
        if ((mem_requirements.memoryTypeBits & (1 << mem_index)) &&
            (mem_properties.memoryTypes[mem_index].propertyFlags &
            desc.properties) == desc.properties)
        {
            break;
        }
    }

    VkMemoryAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_requirements.size;
    alloc_info.memoryTypeIndex = mem_index;

    err = vkAllocateMemory(device, &alloc_info, nullptr, &memory_);
    ALWAYS_ASSERT(!err, "Failed to allocate memory for Vulkan Image");

    vkBindImageMemory(device, image_, memory_, 0);

    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image_;
    viewInfo.format = format_;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;

    viewInfo.subresourceRange.aspectMask = aspect_mask_;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = mip_levels_;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = array_layers_;

    viewInfo.components.r = VK_COMPONENT_SWIZZLE_R;
    viewInfo.components.g = VK_COMPONENT_SWIZZLE_G;
    viewInfo.components.b = VK_COMPONENT_SWIZZLE_B;
    viewInfo.components.a = VK_COMPONENT_SWIZZLE_A;

    err = vkCreateImageView(device, &viewInfo, nullptr, &view_);
    ALWAYS_ASSERT(!err, "Failed to create Vulkan Image View");
}

void Image::transitionLayout(VkImageLayout target)
{
    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = layout_;
    barrier.newLayout = target;
    // barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    // barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.srcQueueFamilyIndex = hk::context()->graphics().family();
    barrier.dstQueueFamilyIndex = hk::context()->graphics().family();
    barrier.image = image_;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = mip_levels_;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = array_layers_;

    barrier.srcAccessMask = getAccessFlags(layout_, true);
    barrier.dstAccessMask = getAccessFlags(target, false);

    VkPipelineStageFlags src_stage = getPipelineStageFlags(layout_, true);
    VkPipelineStageFlags dst_stage = getPipelineStageFlags(target, false);

    if (target == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
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
            src_stage, dst_stage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );
    });

    aspect_mask_ = barrier.subresourceRange.aspectMask;
    layout_ = target;
}

VkPipelineStageFlags Image::getPipelineStageFlags(VkImageLayout layout, b8 src)
{
    VkPipelineStageFlags out;

    switch (layout) {
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL: {
        out = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL: {
        out = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL: {
        out =
            VK_PIPELINE_STAGE_VERTEX_SHADER_BIT   |
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    }
    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: {
        out = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    }
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL: {
        out =
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
            VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    }
    case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR: {
        out = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    }
    case VK_IMAGE_LAYOUT_GENERAL: {
        out = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    }
    default: {
        if (src) {
            out = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;  // src
        } else {
            out = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT; // dst
        }
    }
    }

    return out;
}

VkAccessFlags Image::getAccessFlags(VkImageLayout layout, b8 src)
{
    VkAccessFlags out;

    switch (layout) {
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL: {
        out = VK_ACCESS_TRANSFER_READ_BIT;
    }
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL: {
        out = VK_ACCESS_TRANSFER_WRITE_BIT;
    }
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL: {
        out = VK_ACCESS_SHADER_READ_BIT;
    }
    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: {
        out = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        if (!src) {
            out |= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT; // dst
        }
    }
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL: {
        out = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        if (!src) {
            out |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT; // dst
        }
    }
    case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR: {
        out = VK_ACCESS_MEMORY_READ_BIT;
    }
    case VK_IMAGE_LAYOUT_GENERAL: {
        out = VK_ACCESS_HOST_READ_BIT | VK_ACCESS_HOST_WRITE_BIT;
    }
    default: {
        out = 0;
    }
    }

    return out;
}

}
