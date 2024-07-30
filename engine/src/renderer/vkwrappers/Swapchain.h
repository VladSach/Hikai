#ifndef HK_SWAPCHAIN_H
#define HK_SWAPCHAIN_H

#include "vendor/vulkan/vulkan.h"

#include "renderer/VulkanContext.h"

#include "math/utils.h"
#include "utils/containers/hkvector.h"

namespace hk {

class Swapchain {
public:
    void init(VkSurfaceKHR surface, VkExtent2D size);
    void deinit();

    VkResult acquireNextImage(VkSemaphore semaphore, u32 &index);
    VkResult present(u32 index, VkSemaphore semaphore);

public:
    constexpr VkSwapchainKHR handle() const { return handle_; }
    constexpr VkFormat format() const { return format_; }

    constexpr hk::vector<VkImage> &images() { return images_; }
    constexpr hk::vector<VkImageView> &views() { return views_; }

    constexpr VkExtent2D extent() const { return extent_; }

private:
    VkSwapchainKHR handle_ = VK_NULL_HANDLE;
    VkFormat format_;

    // TODO: chane to Image class
    hk::vector<VkImage> images_;
    hk::vector<VkImageView> views_;

    VkExtent2D extent_;
    Queue present_;
};

}

#endif // HK_SWAPCHAIN_H
