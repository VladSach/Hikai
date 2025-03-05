#ifndef HK_IMAGE_H
#define HK_IMAGE_H

#include "vendor/vulkan/vulkan.h"

#include "defines.h"
#include "utils/numerics/hkbitflag.h"

// FIX: temp
#include <string>
#include "renderer/vkwrappers/vkdebug.h"

namespace hk {

class Image {
public:
    enum class Usage {
        NONE = 0,

        TRANSFER_SRC = 1 << 1,
        TRANSFER_DST = 1 << 2,

        SAMPLED = 1 << 3,

        COLOR_ATTACHMENT = 1 << 4,
        DEPTH_STENCIL_ATTACHMENT = 1 << 5,
        INPUT_ATTACHMENT = 1 << 6,
    };

    struct ImageDesc {
        std::string name = "Unknown";
        hk::bitflag<Usage> usage;

        VkFormat format;
        VkImageLayout layout;
        VkImageAspectFlags aspect_mask;

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

    void copy(Image &src);

    void transitionLayout(VkImageLayout target);

    void generateMipmaps();

public:
    constexpr u32 width() const { return width_; }
    constexpr u32 height() const { return height_; }
    constexpr VkFormat format() const { return format_; }
    constexpr VkImage image() const { return image_; } // TODO: remove
    constexpr VkImageView view() const { return view_; }
    constexpr VkImageLayout layout() const { return layout_; }

private:
    void allocateImage(const VulkanImageDesc &desc);

    // Helper methods for layout transition
    VkPipelineStageFlags getPipelineStageFlags(VkImageLayout layout, b8 src);
    VkAccessFlags getAccessFlags(VkImageLayout layout, b8 src);

private:
    u32 width_    = 0;
    u32 height_   = 0;
    u32 channels_ = 0;

    u32 mip_levels_ = 1;
    u32 array_layers_ = 1;

    // Usage usage_ = Usage::NONE;
    hk::bitflag<Usage> usage_ = Usage::NONE;

    VkImage image_         = VK_NULL_HANDLE;
    VkImageView view_      = VK_NULL_HANDLE;
    VkDeviceMemory memory_ = VK_NULL_HANDLE;

    VkFormat format_ = VK_FORMAT_UNDEFINED;
    VkImageLayout layout_ = VK_IMAGE_LAYOUT_UNDEFINED;
    VkImageAspectFlags aspect_mask_ = VK_IMAGE_ASPECT_NONE;
};

REGISTER_ENUM(Image::Usage);

}

#endif // HK_IMAGE_H
