#include "Swapchain.h"

#ifdef HKWINDOWS
#include "vendor/vulkan/vulkan_win32.h"
#endif

#include "renderer/VulkanContext.h"
#include "renderer/vkwrappers/vkdebug.h"

#include "math/utils.h"

namespace hk {

void Swapchain::init(const Window *window)
{
    instance_ = hk::context()->instance();
    device_   = hk::context()->device();
    physical_ = hk::context()->physical();

    surface_format_ = {};
    present_mode_ = VK_PRESENT_MODE_MAX_ENUM_KHR;
    surf_info_ = {};

    createSurface(window);
}

void Swapchain::deinit()
{
    if (handle_) {
        for (auto view : views_) {
            vkDestroyImageView(device_, view, nullptr);
            view = VK_NULL_HANDLE;
        }

        vkDestroySwapchainKHR(device_, handle_, nullptr);
    }

    if (surface_) {
        vkDestroySurfaceKHR(context()->instance(), surface_, nullptr);
    }

    handle_ = VK_NULL_HANDLE;
    surface_ = VK_NULL_HANDLE;

    instance_ = VK_NULL_HANDLE;
    device_ = VK_NULL_HANDLE;
    physical_ = VK_NULL_HANDLE;
}

void Swapchain::recreate(const VkExtent2D &size,
                         const VkSurfaceFormatKHR &preferred_format,
                         const VkPresentModeKHR &preferred_mode)
{
    VkResult err;

    if (!surface_format_.format) {
        setSurfaceFormat(preferred_format);
    }

    if (present_mode_ == VK_PRESENT_MODE_MAX_ENUM_KHR) {
        setPresentMode(preferred_mode);
    }

    // Set swapchain size
    extent_.width = hkm::clamp(
        size.width,
        surf_info_.caps.minImageExtent.width,
        surf_info_.caps.maxImageExtent.width
    );
    extent_.height = hkm::clamp(
        size.height,
        surf_info_.caps.minImageExtent.height,
        surf_info_.caps.maxImageExtent.height
    );

    // Set swapchain images amount
    u32 image_count = surf_info_.caps.minImageCount + 1;
    if (image_count > surf_info_.caps.maxImageCount) {
        image_count = surf_info_.caps.maxImageCount;
    }

    VkSwapchainCreateInfoKHR swapchain_info = {};
    swapchain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_info.surface = surface_;
    swapchain_info.presentMode = present_mode_;
    swapchain_info.imageFormat = surface_format_.format;
    swapchain_info.imageColorSpace = surface_format_.colorSpace;
    swapchain_info.imageExtent = extent_;
    swapchain_info.imageArrayLayers = 1;
    swapchain_info.minImageCount = image_count;
    swapchain_info.preTransform = surf_info_.caps.currentTransform;
    swapchain_info.clipped = VK_TRUE;
    swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;

    // Set possible swapchain image usage
    swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    // Enable transfer source on swapchain images if supported
    if (surf_info_.caps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) {
        swapchain_info.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }

    // Enable transfer destination on swapchain images if supported
    if (surf_info_.caps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
        swapchain_info.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }

    // Helps with swapchain recreation
    VkSwapchainKHR old_swapchain = handle_;
    swapchain_info.oldSwapchain = old_swapchain;

    err = vkCreateSwapchainKHR(device_, &swapchain_info, 0, &handle_);
    ALWAYS_ASSERT(!err, "Failed to create Vulkan Swapchain");

    if (old_swapchain) {
        for (auto view : views_) {
            vkDestroyImageView(device_, view, nullptr);
        }
        vkDestroySwapchainKHR(device_, old_swapchain, nullptr);
    }

    // Get swapchain images
    u32 scImageCount = 0;
    vkGetSwapchainImagesKHR(device_, handle_, &scImageCount, nullptr);

    images_.resize(scImageCount);
    vkGetSwapchainImagesKHR(device_, handle_, &scImageCount, images_.data());

    // Create Image Views
    views_.resize(scImageCount);
    for (u32 i = 0; i < images_.size(); i++) {
        VkImageViewCreateInfo viewInfo = {};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = images_[i];
        viewInfo.format = surface_format_.format;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.flags = 0;

        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        viewInfo.components.r = VK_COMPONENT_SWIZZLE_R;
        viewInfo.components.g = VK_COMPONENT_SWIZZLE_G;
        viewInfo.components.b = VK_COMPONENT_SWIZZLE_B;
        viewInfo.components.a = VK_COMPONENT_SWIZZLE_A;

        err = vkCreateImageView(device_, &viewInfo, nullptr, &views_[i]);
        ALWAYS_ASSERT(!err, "Failed to create Vulkan Image View");

        hk::debug::setName(images_[i], "Swapchain Image #" + std::to_string(i));
        hk::debug::setName(views_[i],
                           "Swapchain Image View #" + std::to_string(i));
    }
}

VkResult Swapchain::acquireNextImage(VkSemaphore semaphore, u32 &index)
{
    return vkAcquireNextImageKHR(device_, handle_, UINT32_MAX, semaphore, 0, &index);
}

VkResult Swapchain::present(u32 index, VkSemaphore semaphore)
{
    return present_.present(handle_, index, semaphore);
}

void Swapchain::setSurfaceFormat(const VkSurfaceFormatKHR &preferred_format)
{
    for (const auto &format : surf_info_.formats) {
        if (format.format     == preferred_format.format &&
            format.colorSpace == preferred_format.colorSpace)
        {
            surface_format_ = format;
            return;
        }
    }

    LOG_DEBUG("Preferred Surface Format is not supported");

    surface_format_ = surf_info_.formats[0];
}

void Swapchain::setPresentMode(const VkPresentModeKHR &preferred_mode)
{
    for (const auto &mode : surf_info_.present_modes) {
        if (mode == preferred_mode) {
            present_mode_ = mode;
            return;
        }
    }

    LOG_DEBUG("Preferred Present Mode is not supported");

    // FIFO mode is always supported
    present_mode_ = VK_PRESENT_MODE_FIFO_KHR;
}

void Swapchain::createSurface(const Window *window)
{
    VkResult err;

#ifdef HKWINDOWS
    const Window *win = static_cast<const Window *>(window);
    VkWin32SurfaceCreateInfoKHR info = {};
    info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    info.hwnd = win->hwnd();
    info.hinstance = win->instance();
    err = vkCreateWin32SurfaceKHR(instance_, &info, 0, &surface_);
#endif // HKWINDOWS

    ALWAYS_ASSERT(!err, "Failed to create Vulkan Surface");

    // Get supported surface formats
    u32 count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_, surface_, &count, nullptr);

    surf_info_.formats.resize(count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_, surface_, &count,
                                         surf_info_.formats.data());

    ALWAYS_ASSERT(count, "Failed to find Vulkan Surface formats");

    // Get supported surface present modes
    count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physical_, surface_, &count,
                                              nullptr);

    surf_info_.present_modes.resize(count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physical_, surface_, &count,
                                              surf_info_.present_modes.data());

    ALWAYS_ASSERT(count, "Failed to find Vulkan Surface present modes");

    // Get surface properties
    surf_info_.caps = {};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_, surface_,
                                              &surf_info_.caps);

    // Get present queue
    // if (!present_.handle()) {
    //     hk::QueueFamily family;
    //     family.findQueue(physical_, VK_QUEUE_GRAPHICS_BIT, surface_);
    //     present_.init(device_, family);
    // }

    // PERF: Currently, swapchain created with assumption that
    // graphics queue also supports present and there is no present only queue.
    // This also means that the swapchain sharing mode is exclusive.
    // Whether this impacts performance require further profiling
    present_ = hk::context()->graphics();

}

}
