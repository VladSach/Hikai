#include "Swapchain.h"

namespace hk {

void Swapchain::getPhysicalInfo(VkPhysicalDevice physical, VkSurfaceKHR surface)
{
    u32 formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical, surface,
                                         &formatCount, nullptr);

    surfaceFormats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical, surface,
                                         &formatCount, surfaceFormats.data());

    u32 presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physical, surface,
                                              &presentModeCount, nullptr);

    presentModes.resize(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physical, surface,
                                              &presentModeCount,
                                              presentModes.data());
}

void Swapchain::setSurfaceFormat(const VkSurfaceFormatKHR &preferredFormat)
{
    for (const auto &format : surfaceFormats) {
        if (format.format     == preferredFormat.format &&
            format.colorSpace == preferredFormat.colorSpace)
        {
            surfaceFormat = format;
            return;
        }
    }

    LOG_DEBUG("Preferred Surface Format is not supported");
    surfaceFormat = surfaceFormats[0];
}

void Swapchain::setPresentMode(const VkPresentModeKHR &preferredMode)
{
    for (const auto &mode : presentModes) {
        if (mode == preferredMode) {
            presentMode = mode;
            return;
        }
    }

    LOG_DEBUG("Preferred Present Mode is not supported");
    // FIFO mode is the one that is always supported
    presentMode = VK_PRESENT_MODE_FIFO_KHR;
}

void Swapchain::init(VkSurfaceKHR surface, VkExtent2D size)
{
    VkResult err;

    VkDevice device = hk::context()->device();
    VkPhysicalDevice physical = hk::context()->physical();

    if (!surfaceFormats.size()) {
        getPhysicalInfo(physical, surface);
    }

    if (!surfaceFormat.format) {
        VkSurfaceFormatKHR preferredFormat = {
            VK_FORMAT_B8G8R8A8_UNORM,
            VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
        };

        setSurfaceFormat(preferredFormat);
    }

    if (presentMode == VK_PRESENT_MODE_MAX_ENUM_KHR) {
        setPresentMode(VK_PRESENT_MODE_MAILBOX_KHR);
    }

    surfaceCaps = {};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical, surface, &surfaceCaps);

    extent_ = size;
    extent_.width = hkm::clamp(extent_.width,
                              surfaceCaps.minImageExtent.width,
                              surfaceCaps.maxImageExtent.width);
    extent_.height = hkm::clamp(extent_.height,
                               surfaceCaps.minImageExtent.height,
                               surfaceCaps.maxImageExtent.height);

    u32 imageCount = surfaceCaps.minImageCount + 1 < surfaceCaps.maxImageCount
                                             ? surfaceCaps.minImageCount + 1
                                             : surfaceCaps.maxImageCount;

    VkSwapchainCreateInfoKHR swapchainInfo = {};
    swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainInfo.surface = surface;
    swapchainInfo.imageFormat = surfaceFormat.format;
    swapchainInfo.imageColorSpace = surfaceFormat.colorSpace;
    swapchainInfo.preTransform = surfaceCaps.currentTransform;
    swapchainInfo.imageExtent = extent_;
    swapchainInfo.minImageCount = imageCount;
    swapchainInfo.imageArrayLayers = 1;
    swapchainInfo.presentMode = presentMode;
    swapchainInfo.clipped = VK_TRUE;
    swapchainInfo.imageUsage =
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
        VK_IMAGE_USAGE_TRANSFER_DST_BIT;


    // TODO: I assume, as there is no present only family, that
    // it will be one of already created families.
    // There is sure a better way
    if (!present_.handle()) {
        hk::QueueFamily family;
        family.findQueue(physical, VK_QUEUE_GRAPHICS_BIT, surface);
        present_.init(device, family);
    }

    // FIX: temp
    // u32 queueFamilyIndices[] = { graphicsFamily, computeFamily,
    //                              transferFamily, presentFamily };
    //
    // if (graphicsFamily != presentFamily) {
    //     swapchainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    //     swapchainInfo.queueFamilyIndexCount = 2;
    //     swapchainInfo.pQueueFamilyIndices = queueFamilyIndices;
    // } else {
    //     swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    // }
    swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;

    swapchainInfo.oldSwapchain = handle_;

    if (handle_) {
        vkDeviceWaitIdle(device);
        for (auto imageView : views_) {
            vkDestroyImageView(device, imageView, nullptr);
            imageView = nullptr;
        }
    }

    err = vkCreateSwapchainKHR(device, &swapchainInfo, 0, &handle_);
    ALWAYS_ASSERT(!err, "Failed to create Vulkan Swapchain");

    u32 scImageCount = 0;
    vkGetSwapchainImagesKHR(device, handle_, &scImageCount, nullptr);

    images_.resize(scImageCount);
    vkGetSwapchainImagesKHR(device, handle_,
                            &scImageCount, images_.data());

    format_ = surfaceFormat.format;

    // Create Image Views
    views_.resize(images_.size());
    for (u32 i = 0; i < images_.size(); i++) {
        VkImageViewCreateInfo viewInfo = {};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = images_[i];
        viewInfo.format = format_;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;
        viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        err = vkCreateImageView(device, &viewInfo, nullptr, &views_[i]);
        ALWAYS_ASSERT(!err, "Failed to create Vulkan Image View");
    }
}

void Swapchain::deinit()
{
    VkDevice device = hk::context()->device();

    vkDeviceWaitIdle(device);

    for (auto imageView : views_) {
        vkDestroyImageView(device, imageView, nullptr);
        imageView = nullptr;
    }

    if (handle_) {
        vkDestroySwapchainKHR(device, handle_, nullptr);
        handle_ = nullptr;
    }
}

VkResult Swapchain::acquireNextImage(VkSemaphore semaphore, u32 &index)
{
    VkDevice device = hk::context()->device();

    return vkAcquireNextImageKHR(device, handle_, 0, semaphore, 0, &index);
}

VkResult Swapchain::present(u32 index, VkSemaphore semaphore)
{
    return present_.present(handle_, index, semaphore);
}

}
