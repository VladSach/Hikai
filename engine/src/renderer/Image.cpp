#include "Image.h"

// FIX: temp
#include "renderer/Buffer.h"
#include "renderer/UBManager.h"
#include "renderer/Descriptors.h"

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

    RenderDevice::ImageDesc imageDesc = {
        width_, height_,
        format_,
        VK_IMAGE_TILING_OPTIMAL,
        flags,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    };

    RenderDevice::DeviceImage deviceImage =
        hk::device()->createImage(imageDesc);

    texture_ = deviceImage.image;
    memory_ = deviceImage.imageMemory;

    // FIX: temp fix, later make all transitions possible
    VkImageLayout tmpLayout = layout_;
    layout_ = (layout_ == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) ?
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL : layout_;

    hk::device()->transitionImageLayout(
        texture_,
        format_,
        VK_IMAGE_LAYOUT_UNDEFINED,
        layout_);

    layout_ = tmpLayout;

    VkDevice device = hk::device()->logical();

    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = texture_;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format_;
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

    VkDevice device = hk::device()->logical();

    if (sampler_)
        vkDestroySampler(device, sampler_, nullptr);
    if (view_)
        vkDestroyImageView(device, view_, nullptr);

    if (texture_)
        vkDestroyImage(device, texture_, nullptr);
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
    // hk::device()->transitionImageLayout(
    //     texture_,
    //     format_,
    //     layout_,
    //     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    hk::device()->copyBufferToImage(
        stagingBuffer.buffer(), texture_, width_, height_);

    hk::device()->transitionImageLayout(
        texture_,
        format_,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        layout_);

    // stagingBuffer.deinit();
}

void Image::bind()
{
    VkResult err;
    VkDevice device = hk::device()->logical();

    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

    VkPhysicalDeviceProperties properties = {};
    vkGetPhysicalDeviceProperties(hk::device()->physical(), &properties);

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

}
