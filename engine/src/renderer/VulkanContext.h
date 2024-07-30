#ifndef HK_VULKAN_CONTEXT_H
#define HK_VULKAN_CONTEXT_H

#include "vendor/vulkan/vulkan.h"

#include "renderer/vkwrappers/Queue.h"

#include "defines.h"

#include <functional>

namespace hk {

class VulkanContext {
public:
    void init();
    void deinit();

    void submitImmCmd(
        const std::function<void(VkCommandBuffer cmd)> &&func);

public:
    constexpr VkInstance       instance() const { return instance_; }
    constexpr VkPhysicalDevice physical() const { return physicalDevice_; }
    constexpr VkDevice         device()   const { return device_; }

    constexpr Queue& graphics() { return graphics_; }
    constexpr Queue& compute() { return compute_; }
    constexpr Queue& transfer() { return transfer_; }

private:
    void createInstance();
    void createPhysicalDevice();
    void createLogicalDevice();

private:
    VkInstance instance_ = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
    VkDevice device_ = VK_NULL_HANDLE;

    Queue graphics_;
    Queue compute_;
    Queue transfer_;

    /* TODO: add debug functionality
     * https://github.com/KhronosGroup/Vulkan-Samples/tree/main/
     * samples/extensions/debug_utils */
    b8 debugUtils = false;
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
};

VulkanContext *context();

}

#endif // HK_VULKAN_CONTEXT_H
