#ifndef HK_VULKAN_CONTEXT_H
#define HK_VULKAN_CONTEXT_H

#include "vendor/vulkan/vulkan.h"

#include "renderer/vkwrappers/Queue.h"
#include "utils/containers/hkvector.h"

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
    constexpr hk::vector<PhysicalDeviceInfo>& physicalInfos()
    {
        return physicalDevicesInfo_;
    }
    constexpr LogicalDeviceInfo& deviceInfo() { return deviceInfo_; }

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

VulkanContext *context();

// TODO: probably should create a separate file for util functions
inline std::string vkApiToString(u32 version)
{
    std::string out;

    out += std::to_string(VK_API_VERSION_MAJOR(version));
    out += '.';
    out += std::to_string(VK_API_VERSION_MINOR(version));
    out += '.';
    out += std::to_string(VK_API_VERSION_PATCH(version));

    return out;
}

inline std::string vkDeviceTypeToString(VkPhysicalDeviceType type)
{
    constexpr const char* lookup_type[] = {
        "Other",
        "Integrated GPU",
        "Discrete GPU",
        "Virtual GPU",
        "CPU",
    };

    return lookup_type[static_cast<u32>(type)];
}

inline std::string vkDeviceDriveToString(u32 version, u32 vendorID)
{
    // Source:
    // https://github.com/SaschaWillems/vulkan.gpuinfo.org/blob/master/
    // includes/functions.php#L414

    std::string out;

    if (vendorID == 0x10DE /* NVIDIA */) {
        out += std::to_string((version >> 22) & 0x3ff);
        out += '.';
        out += std::to_string((version >> 14) & 0x0ff);
        out += '.';
        out += std::to_string((version >> 6) & 0x0ff);
        out += '.';
        out += std::to_string((version) & 0x003f);
    } else if (vendorID == 0x8086 /* INTEL */) {
        out += std::to_string(version >> 14);
        out += '.';
        out += std::to_string(version & 0x3fff);
    } else {
        // Use Vulkan version conventions if vendor mapping is not available
        out += std::to_string(VK_API_VERSION_MAJOR(version));
        out += '.';
        out += std::to_string(VK_API_VERSION_MINOR(version));
        out += '.';
        out += std::to_string(VK_API_VERSION_PATCH(version));
    }

    return out;
}

inline std::string vkDeviceVendorToString(u32 id)
{
    // Not gonna write all that
    // https://pcisig.com/membership/member-companies

    std::string out;

    switch (id) {
    case 0x1002: {
        out = "AMD";
    } break;

    case 0x10DE: {
        out = "NVIDIA";
    } break;

    case 0x13B5: {
        out = "ARM";
    } break;

    case 0x5143: {
        out = "Qualcomm";
    } break;

    case 0x8086: {
        out = "INTEL";
    } break;

    default:
        out = "Other";
    }

    return out;
}

}

#endif // HK_VULKAN_CONTEXT_H
