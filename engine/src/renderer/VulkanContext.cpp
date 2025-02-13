#include "VulkanContext.h"

#include "platform/platform.h"

#ifdef HKWINDOWS
#include "vendor/vulkan/vulkan_win32.h"
#endif

#include "renderer/vkwrappers/vkdebug.h"

#include "utils/containers/hkvector.h"

// TODO: change with hikai implementation
#include <set>

namespace hk {

VulkanContext *context()
{
    static hk::VulkanContext *globalContext = nullptr;

    if (globalContext == nullptr) {
        globalContext = new hk::VulkanContext();
    }
    return globalContext;
}

void VulkanContext::init()
{
    LOG_INFO("Initializing Vulkan Context");

    getInstanceInfo();
    createInstance();

    getPhysicalDeviceInfo();
    createPhysicalDevice();

    createLogicalDevice();

    hk::debug::setName(instance_,  "Instance");
    hk::debug::setName(device_,    "Logical Device");
    hk::debug::setName(physical(), "Physical Device");
    // for (const auto &info : physicalDevicesInfo_) {
        // hk::debug::setName(info.device, info.properties.deviceName);
    // }

    hk::debug::setName(graphics_.handle(), "Graphics Queue");
    hk::debug::setName(compute_.handle(),  "Compute Queue");
    hk::debug::setName(transfer_.handle(), "Transfer Queue");
}

void VulkanContext::deinit()
{
    transfer_.deinit();
    compute_.deinit();
    graphics_.deinit();

    if (device_)
        vkDestroyDevice(device_, nullptr);

    hk::debug::deinit(instance_);

    if (instance_)
        vkDestroyInstance(instance_, nullptr);
}

void VulkanContext::submitImmCmd(
    const std::function<void(VkCommandBuffer cmd)> &&callback)
{
    VkResult err;

    VkCommandBuffer immCommandBuffer_ = graphics_.createCommandBuffer();
    hk::debug::setName(immCommandBuffer_, "Imm Command Buffer");

    // err = vkResetCommandBuffer(immCommandBuffer_, 0);
    // ALWAYS_ASSERT(!err, "Failed to reset Vulkan immediate Command Buffer");

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    err = vkBeginCommandBuffer(immCommandBuffer_, &beginInfo);
    ALWAYS_ASSERT(!err, "Failed to begin immediate Command Buffer");

    callback(immCommandBuffer_);

    err = vkEndCommandBuffer(immCommandBuffer_);
    ALWAYS_ASSERT(!err, "Failed to end immediate Command Buffer");

    graphics_.submitAndWait(immCommandBuffer_);

    graphics_.freeCommandBuffer(immCommandBuffer_);
}

void VulkanContext::getInstanceInfo()
{
    vkEnumerateInstanceVersion(&instanceInfo_.apiVersion);

    u32 extCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extCount, nullptr);
    hk::vector<VkExtensionProperties> extensions(extCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extCount, extensions.data());

    // instanceInfo_.extensions.resize(extCount);
    for (const auto &ext : extensions) {
        instanceInfo_.extensions.push_back({ false, ext });
    }

    u32 lCount = 0;
    vkEnumerateInstanceLayerProperties(&lCount, nullptr);
    hk::vector<VkLayerProperties> layers(lCount);
    vkEnumerateInstanceLayerProperties(&lCount, layers.data());

    // instanceInfo_.layers.resize(lCount);
    for (const auto &layer : layers) {
        instanceInfo_.layers.push_back({ false, layer });
    }

    // TODO: make configurable?
    hk::vector<const char *> requiredExts = {
        VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME,
        VK_KHR_SURFACE_EXTENSION_NAME,

#ifdef HKWINDOWS
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#endif
    };

#ifdef HKDEBUG
        instanceInfo_.isDebug = true;
        requiredExts.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

    for (auto &ext : instanceInfo_.extensions) {
        for (const auto &required : requiredExts) {
            if (!strcmp(ext.second.extensionName, required)) {
                ext.first = true;
            }
        }
    }

    hk::vector<const char *> requiredLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    for (auto &layer : instanceInfo_.layers) {
        for (const auto &required : requiredLayers) {
            if (!strcmp(layer.second.layerName, required)) {
                layer.first = true;
            }
        }
    }
}

void VulkanContext::getPhysicalDeviceInfo()
{
    u32 deviceCount = 0;
    vkEnumeratePhysicalDevices(instance_, &deviceCount, nullptr);
    ALWAYS_ASSERT(deviceCount, "Failed to find GPUs with Vulkan support");

    hk::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance_, &deviceCount, devices.data());

    physicalDevicesInfo_.resize(deviceCount);

    VkPhysicalDeviceFeatures deviceFeatures;
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceMemoryProperties deviceMemoryProperties;

    for (u32 i = 0; i < devices.size(); ++i) {
        const auto &device = devices.at(i);

        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
        vkGetPhysicalDeviceProperties(device, &deviceProperties);
        vkGetPhysicalDeviceMemoryProperties(device, &deviceMemoryProperties);

        u32 queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
        hk::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        u32 extensionCount = 0;
        vkEnumerateDeviceExtensionProperties(device, 0,  &extensionCount, nullptr);
        hk::vector<VkExtensionProperties> extensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, 0,  &extensionCount, extensions.data());

        PhysicalDeviceInfo &info = physicalDevicesInfo_.at(i);
        info.device = device;
        info.features = deviceFeatures;
        info.properties = deviceProperties;
        info.memProperties = deviceMemoryProperties;
        info.families = queueFamilies;
        info.extensions = extensions;
    }
}

void VulkanContext::createInstance()
{
    VkResult err;

    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    // TODO: make configurable
    // appInfo.pApplicationName = "Sandbox";
    appInfo.pEngineName = "Hikai";
    appInfo.apiVersion = instanceInfo_.apiVersion;

    hk::vector<const char *> extensionsOn;
    hk::vector<const char *> layersOn;

    for (const auto &ext : instanceInfo_.extensions) {
        if (ext.first) {
            extensionsOn.push_back(ext.second.extensionName);
        }
    }

    for (const auto &layer : instanceInfo_.layers) {
        if (layer.first) {
            layersOn.push_back(layer.second.layerName);
        }
    }

    VkInstanceCreateInfo instanceInfo = {};
    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pApplicationInfo = &appInfo;
    instanceInfo.ppEnabledExtensionNames = extensionsOn.data();
    instanceInfo.enabledExtensionCount = extensionsOn.size();
    instanceInfo.ppEnabledLayerNames = layersOn.data();
    instanceInfo.enabledLayerCount = layersOn.size();

    if (instanceInfo_.isDebug) {
        instanceInfo.pNext = &hk::debug::info();
    }

    err = vkCreateInstance(&instanceInfo, nullptr, &instance_);
    ALWAYS_ASSERT(!err, "Failed to create Vulkan Instance");

    if (instanceInfo_.isDebug) {
        hk::debug::init(instance_);
    }
}

void VulkanContext::createPhysicalDevice()
{
    // TODO: improve device picking
    for (u32 i = 0; i < physicalDevicesInfo_.size(); ++i) {
        if (physicalDevicesInfo_[i].properties.deviceType ==
            VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            physicalDeviceIndex_ = i;
        }
    }

    ALWAYS_ASSERT(physicalDeviceIndex_ != -1, "Failed to find a suitable GPU");
}

void VulkanContext::createLogicalDevice()
{
    VkResult err;

    deviceInfo_.graphicsFamily.findQueue(physical(), VK_QUEUE_GRAPHICS_BIT);
    deviceInfo_.computeFamily.findQueue(physical(), VK_QUEUE_COMPUTE_BIT);
    deviceInfo_.transferFamily.findQueue(physical(), VK_QUEUE_TRANSFER_BIT);

    f32 queuePriority = 1.f;
    hk::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

    if (deviceInfo_.graphicsFamily) {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = deviceInfo_.graphicsFamily.index_;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }
    if (deviceInfo_.computeFamily) {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = deviceInfo_.computeFamily.index_;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }
    if (deviceInfo_.transferFamily) {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = deviceInfo_.transferFamily.index_;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    ALWAYS_ASSERT(queueCreateInfos.size(), "Failed to find at least one queue");

    // Create logical device
    hk::vector<const char *> extensionsLogic = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };

    VkPhysicalDeviceFeatures supportedFeatures =
        physicalDevicesInfo_[physicalDeviceIndex_].features;

    VkPhysicalDeviceFeatures requiredFeatures = {};
    if (supportedFeatures.samplerAnisotropy) {
        requiredFeatures.samplerAnisotropy = VK_TRUE;
    }

    VkDeviceCreateInfo deviceInfo = {};
    deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceInfo.pQueueCreateInfos = queueCreateInfos.data();
    deviceInfo.queueCreateInfoCount = static_cast<u32>(queueCreateInfos.size());
    deviceInfo.ppEnabledExtensionNames = extensionsLogic.data();
    deviceInfo.enabledExtensionCount = extensionsLogic.size();
    deviceInfo.pEnabledFeatures = &requiredFeatures;

    err = vkCreateDevice(physical(), &deviceInfo, 0, &device_);
    ALWAYS_ASSERT(!err, "Failed to create Vulkan Logical Device");

    hk::debug::setDevice(device_);

    // Get queue handles
    graphics_.init(device_, deviceInfo_.graphicsFamily);
    compute_.init(device_, deviceInfo_.computeFamily);
    transfer_.init(device_, deviceInfo_.transferFamily);
}

}

