#ifndef HK_VKCONTEXT_H
#define HK_VKCONTEXT_H

#include "hkcommon.h"

#include "vendor/vulkan/vulkan.h"
#include "renderer/vkwrappers/Queue.h"

#include "hkstl/containers/hkvector.h"

#include <functional>

namespace hk {

struct InstanceInfo {
    u32 api; // version
    b8 is_debug = false;
    // TODO: change to just vector of turned on ext, or layers
    // To dynamically requiest new ext/layer add separate function
    hk::vector<std::pair<b8, VkExtensionProperties>> extensions;
    hk::vector<std::pair<b8, VkLayerProperties>> layers;
};

struct PhysicalDeviceInfo {
    VkPhysicalDevice device;
    VkPhysicalDeviceFeatures features;
    VkPhysicalDeviceProperties  properties;
    VkPhysicalDeviceMemoryProperties memProperties;

    hk::vector<VkQueueFamilyProperties> families;
    hk::vector<VkExtensionProperties> extensions;
};

struct LogicalDeviceInfo {
    hk::QueueFamily graphicsFamily;
    hk::QueueFamily computeFamily;
    hk::QueueFamily transferFamily;
};

void init();
void deinit();

void submitImmCmd(const std::function<void(VkCommandBuffer cmd)> &&func);

constexpr VkInstance       instance();
constexpr VkPhysicalDevice physical();
constexpr VkDevice         device();

constexpr InstanceInfo& instanceInfo();
constexpr LogicalDeviceInfo& deviceInfo();
constexpr PhysicalDeviceInfo& physicalInfo();
constexpr hk::vector<PhysicalDeviceInfo>& allPhysicalInfos();

constexpr Queue& graphics();
constexpr Queue& compute();
constexpr Queue& transfer();

void getInstanceInfo();
void getPhysicalDeviceInfo();
void getLogicalDeviceInfo();

void createInstance();
// More like pickPhysicalDevice?
void createPhysicalDevice();
void createLogicalDevice();

}

#endif // HK_VKCONTEXT_H
