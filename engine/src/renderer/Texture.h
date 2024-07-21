#ifndef HK_TEXTURE_H
#define HK_TEXTURE_H

#include "defines.h"
#include "renderer/Buffer.h"
#include "vendor/vulkan/vulkan.h"

// FIX: temp
#include "renderer/UBManager.h"
#include "renderer/Descriptors.h"

namespace hk {

class Texture {
public:
    // constexpr b8 operator ==(const Texture &other) const
    // {
    //     return (
    //         this->texture_ == other.texture_ &&
    //         this->memory_  == other.memory_
    //     );
    // }

    Texture() :
        width_(0), height_(0),
        channels_(0), pixels_(nullptr)
    {}
    Texture(void *pixels, i32 width, i32 height, i32 channels) :
        width_(width), height_(height),
        channels_(channels), pixels_(pixels)
    {}

    ~Texture()
    {
        deinit();
    }

    void init(void *pixels, i32 width, i32 height, i32 channels)
    {
        width_ = width;
        height_ = height;
        channels_ = channels;
        pixels_ = pixels;
    }

    void deinit()
    {
        width_ = 0;
        height_ = 0;
        channels_ = 0;
        // FIX: not sure what to do here, shoud copy and free and loader maybe?
        free(pixels_);

        VkDevice device = hk::device()->logical();

        vkDestroyImageView(device, view_, nullptr);

        vkDestroyImage(device, texture_, nullptr);
        vkFreeMemory(device, memory_, nullptr);
    }

    void writeToBuffer()
    {
        VkResult err;

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

        stagingBuffer.update(pixels_);

        VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;
        RenderDevice::ImageDesc imageDesc = {
            width_, height_,
            format,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT |
            VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        };

        RenderDevice::DeviceImage deviceImage =
            hk::device()->createImage(imageDesc);

        texture_ = deviceImage.image;
        memory_ = deviceImage.imageMemory;

        hk::device()->transitionImageLayout(
            texture_,
            format,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        hk::device()->copyBufferToImage(
            stagingBuffer.buffer(), texture_, width_, height_);

        hk::device()->transitionImageLayout(
            texture_,
            format,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        // stagingBuffer.deinit();

        VkImageViewCreateInfo viewInfo = {};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = texture_;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VkDevice device = hk::device()->logical();
        err = vkCreateImageView(device, &viewInfo, nullptr, &view_);
        ALWAYS_ASSERT(!err, "Failed to create Vulkan Image View");

        VkSamplerCreateInfo samplerInfo = {};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

        VkPhysicalDeviceProperties properties {};
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
    }

    void bind() {
        // FIX: temp
        hk::DescriptorWriter *writer = getGlobalDescriptorWriter();
        writer->writeImage(1, view_, sampler_,
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    }

private:
    u32 width_;
    u32 height_;
    u32 channels_;
    void *pixels_;

    VkImage texture_;
    VkImageView view_;
    VkSampler sampler_;
    VkDeviceMemory memory_;
};

}

#endif // HK_TEXTURE_H
