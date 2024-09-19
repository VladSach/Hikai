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

    void setSurfaceFormat(const VkSurfaceFormatKHR &preferredFormat);
    void setPresentMode(const VkPresentModeKHR &preferredMode);

public:
    constexpr VkSwapchainKHR handle() const { return handle_; }
    constexpr VkFormat format() const { return format_; }

    constexpr hk::vector<VkImage> &images() { return images_; }
    constexpr hk::vector<VkImageView> &views() { return views_; }

    constexpr VkExtent2D extent() const { return extent_; }

private:
    void getPhysicalInfo(VkPhysicalDevice physical, VkSurfaceKHR surface);

private:
    VkSwapchainKHR handle_ = VK_NULL_HANDLE;
    VkFormat format_;

    // TODO: chane to Image class
    hk::vector<VkImage> images_;
    hk::vector<VkImageView> views_;

    VkExtent2D extent_;
    Queue present_;

    // FIX: temp
public:
    VkSurfaceFormatKHR surfaceFormat = {};
    hk::vector<VkSurfaceFormatKHR> surfaceFormats;
    VkPresentModeKHR presentMode = VK_PRESENT_MODE_MAX_ENUM_KHR;
    hk::vector<VkPresentModeKHR> presentModes;
    VkSurfaceCapabilitiesKHR surfaceCaps;
};

}

#endif // HK_SWAPCHAIN_H
