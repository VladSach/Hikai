#ifndef HK_VULKAN_CONTEXT_H
#define HK_VULKAN_CONTEXT_H

#include "vendor/vulkan/vulkan.h"

#include "defines.h"

#include "renderer/vkwrappers/Queue.h"
#include "utils/containers/hkvector.h"

#include <functional>

namespace hk {

class VulkanContext {
public:
    void init();
    void deinit();

    void submitImmCmd(
        const std::function<void(VkCommandBuffer cmd)> &&func);

public:
    struct InstanceInfo;
    struct PhysicalDeviceInfo;
    struct LogicalDeviceInfo;

public:
    constexpr VkInstance       instance() const { return instance_; }
    constexpr VkPhysicalDevice physical() const
    {
        return physicalDevicesInfo_.at(physicalDeviceIndex_).device;
    }
    constexpr VkDevice         device()   const { return device_; }

    constexpr InstanceInfo& instanceInfo() { return instanceInfo_; }
    constexpr LogicalDeviceInfo& deviceInfo() { return deviceInfo_; }
    constexpr PhysicalDeviceInfo& physicalInfo()
    {
        return physicalDevicesInfo_.at(physicalDeviceIndex_);
    }
    constexpr hk::vector<PhysicalDeviceInfo>& allPhysicalInfos()
    {
        return physicalDevicesInfo_;
    }

    constexpr Queue& graphics() { return graphics_; }
    constexpr Queue& compute() { return compute_; }
    constexpr Queue& transfer() { return transfer_; }

public:
    struct InstanceInfo {
        u32 apiVersion;
        b8 isDebug = false;
        hk::vector<std::pair<b8, VkExtensionProperties>> extensions;
        hk::vector<std::pair<b8, VkLayerProperties>> layers;
    };

    struct PhysicalDeviceInfo {
        VkPhysicalDevice device;
        VkPhysicalDeviceFeatures features;
        VkPhysicalDeviceProperties properties;
        VkPhysicalDeviceMemoryProperties memProperties;

        hk::vector<VkQueueFamilyProperties> families;
        hk::vector<VkExtensionProperties> extensions;
    };

    struct LogicalDeviceInfo {
        hk::QueueFamily graphicsFamily;
        hk::QueueFamily computeFamily;
        hk::QueueFamily transferFamily;
    };

private:
    void getInstanceInfo();
    void getPhysicalDeviceInfo();
    void getLogicalDeviceInfo();

    void createInstance();
    // More like pickPhysicalDevice?
    void createPhysicalDevice();
    void createLogicalDevice();

private:
    VkInstance instance_ = VK_NULL_HANDLE;
    InstanceInfo instanceInfo_;

    i32 physicalDeviceIndex_ = -1;
    hk::vector<PhysicalDeviceInfo> physicalDevicesInfo_;

    LogicalDeviceInfo deviceInfo_;
    VkDevice device_ = VK_NULL_HANDLE;

    Queue graphics_;
    Queue compute_;
    Queue transfer_;
};

// FIX: temp api
HKAPI VulkanContext *context();

}

#endif // HK_VULKAN_CONTEXT_H
