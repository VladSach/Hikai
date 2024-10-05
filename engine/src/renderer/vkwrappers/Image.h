#ifndef HK_IMAGE_H
#define HK_IMAGE_H

#include "vendor/vulkan/vulkan.h"

#include "defines.h"

namespace hk {

class Image {
public:
    enum class Usage {
        NONE = 0,

        TRANSFER_SRC,
        TRANSFER_DST,

        SAMPLED,

        // COLOR_ATTACHMENT,
        DEPTH_STENCIL_ATTACHMENT,

        MAX_IMAGE_USAGE
    };

    struct ImageDesc {
        Usage usage;

        VkFormat format;
        VkImageLayout layout;
        VkImageAspectFlags aspectMask;

        u32 width;
        u32 height;
        u32 channels;
    };

    struct VulkanImageDesc {
        u32 width, height;
        VkFormat format;
        VkImageTiling tiling;
        VkImageUsageFlags usage;
        VkMemoryPropertyFlags properties;
    };

public:
    Image() = default;
    Image(const ImageDesc &desc) { init(desc); }
    ~Image() { deinit(); }

    void init(const ImageDesc &desc);
    void deinit();

    void write(const void *pixels);

    void copy();

public:
    constexpr u32 width() const { return width_; }
    constexpr u32 height() const { return height_; }
    constexpr VkFormat format() const { return format_; }
    constexpr VkImageView view() const { return view_; }
    constexpr VkImageLayout layout() const { return layout_; }

private:
    void allocateImage(const VulkanImageDesc &desc);
    void transitionImageLayout(
        VkImageLayout oldLayout,
        VkImageLayout newLayout);

private:
    u32 width_    = 0;
    u32 height_   = 0;
    u32 channels_ = 0;

    Usage usage_ = Usage::NONE;

    VkImage image_         = VK_NULL_HANDLE;
    VkImageView view_      = VK_NULL_HANDLE;
    VkDeviceMemory memory_ = VK_NULL_HANDLE;

    VkFormat format_ = VK_FORMAT_UNDEFINED;
    VkImageLayout layout_ = VK_IMAGE_LAYOUT_UNDEFINED;
    VkImageAspectFlags aspectMask_ = VK_IMAGE_ASPECT_NONE;
};

constexpr Image::Usage operator |(const Image::Usage a, const Image::Usage b)
{
    return static_cast<Image::Usage>(
        static_cast<u32>(a) | static_cast<u32>(b)
    );
}

constexpr b8 operator &(const Image::Usage a, const Image::Usage b)
{
    return static_cast<u32>(a) & static_cast<u32>(b);
}

}

#endif // HK_IMAGE_H
