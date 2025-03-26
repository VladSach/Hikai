#ifndef HK_VKCONTEXT_H
#define HK_VKCONTEXT_H

#include "hkcommon.h"

#include "vendor/vulkan/vulkan.h"
#include "renderer/vkwrappers/Queue.h"

#include "hkstl/containers/hkvector.h"

#include <functional>
#include <unordered_set>

namespace hk::vkc {

void init();
void deinit();

// FIX: temp
void submitImmCmd(const std::function<void(VkCommandBuffer cmd)> &&func);
VkInstance       instance();
VkPhysicalDevice adapter();
VkDevice         device();
Queue& graphics();
Queue& compute();
Queue& transfer();

struct InstanceInfo {
    u32 api; // version

    hk::vector<std::pair<b8, VkExtensionProperties>> exts;
    hk::vector<std::pair<b8, VkLayerProperties>> layers;
};

struct AdapterInfo {
    VkPhysicalDevice adapter;

    struct Features {
        VkPhysicalDeviceFeatures core;
        VkPhysicalDeviceVulkan11Features v11;
        VkPhysicalDeviceVulkan12Features v12;
        VkPhysicalDeviceVulkan13Features v13;
    } features;
    VkPhysicalDeviceFeatures2 all_features;

    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceMemoryProperties memory_properties;

    hk::vector<VkQueueFamilyProperties> families;
    hk::vector<VkExtensionProperties> exts;
};

struct DeviceInfo {
    // TODO: This can be moved to Adapter info
    hk::QueueFamily graphics_family;
    hk::QueueFamily transfer_family;
    hk::QueueFamily compute_family;
};

HKAPI const InstanceInfo& instance_info();
HKAPI const AdapterInfo&  adapter_info(u32 idx = 0);
HKAPI const DeviceInfo&   device_info();
HKAPI const u32 adapter_count();

}

#endif // HK_VKCONTEXT_H
