#ifndef HK_SWAPCHAIN_H
#define HK_SWAPCHAIN_H

#include "vendor/vulkan/vulkan.h"

#include "platform/platform.h"
#include "renderer/vkwrappers/Queue.h"

#include "hkstl/containers/hkvector.h"

namespace hk {

class Swapchain {
public:
    void init(const Window *window);
    void deinit();

    HKAPI void recreate(
        const VkExtent2D &size = { 0, 0 },
        const VkSurfaceFormatKHR &preferred_format = {},
        const VkPresentModeKHR &preferred_mode = VK_PRESENT_MODE_FIFO_KHR
    );

    VkResult acquireNextImage(VkSemaphore semaphore, u32 &index);
    VkResult present(u32 index, VkSemaphore semaphore);

private:
    struct SurfaceInfo {
        hk::vector<VkSurfaceFormatKHR> formats;
        hk::vector<VkPresentModeKHR> present_modes;
        VkSurfaceCapabilitiesKHR caps;
    };

public:
    constexpr VkSwapchainKHR handle() const { return handle_; }
    constexpr VkFormat format() const { return surface_format_.format; }
    constexpr VkExtent2D extent() const { return extent_; }

    constexpr hk::vector<VkImage> &images() { return images_; }
    constexpr hk::vector<VkImageView> &views() { return views_; }

    constexpr const SurfaceInfo& info() const { return surf_info_; }

private:
    void createSurface(const Window *window);
    void setSurfaceFormat(const VkSurfaceFormatKHR &format);
    void setPresentMode(const VkPresentModeKHR &mode);

private:
    VkSurfaceKHR surface_ = VK_NULL_HANDLE;
    VkSwapchainKHR handle_ = VK_NULL_HANDLE;

    hk::vector<VkImage> images_;
    hk::vector<VkImageView> views_;

    VkExtent2D extent_;
    Queue present_;

    VkSurfaceFormatKHR surface_format_;
    VkPresentModeKHR present_mode_;

    SurfaceInfo surf_info_;

    // Convenience
    VkInstance instance_;
    VkDevice device_;
    VkPhysicalDevice physical_;
};

}

#endif // HK_SWAPCHAIN_H
