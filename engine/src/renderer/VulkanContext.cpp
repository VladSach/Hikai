#include "VulkanContext.h"

#include "platform/platform.h"

#ifdef HKWINDOWS
#include "vendor/vulkan/vulkan_win32.h"
#endif

#include "utils/containers/hkvector.h"

// TODO: change with hikai implementation
#include <set>

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
    void *pUserData);

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

    createInstance();
    createPhysicalDevice();
    createLogicalDevice();
}

void VulkanContext::deinit()
{
    transfer_.deinit();
    compute_.deinit();
    graphics_.deinit();

    vkDestroyDevice(device_, nullptr);

    if (debugUtils) {
        auto vkDestroyDebugUtilsMessengerEXT =
            reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(instance_,
                                  "vkDestroyDebugUtilsMessengerEXT"));
        vkDestroyDebugUtilsMessengerEXT(instance_, debugMessenger, nullptr);
    }

    vkDestroyInstance(instance_, nullptr);
}

void VulkanContext::submitImmCmd(
    const std::function<void(VkCommandBuffer cmd)> &&callback)
{
    VkResult err;

    VkCommandBuffer immCommandBuffer_ = graphics_.createCommandBuffer();

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

void VulkanContext::createInstance()
{
    VkResult err;

    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    // TODO: make configurable
    appInfo.pApplicationName = "Sandbox";
    appInfo.pEngineName = "Hikai";
    appInfo.apiVersion = VK_API_VERSION_1_3;

    u32 extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    hk::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount,
                                           availableExtensions.data());

    // LOG_DEBUG("Supported Vulkan Extensions:");
    // for (auto &extension : availableExtensions) {
    //       LOG_TRACE(extension.extensionName);
    // }

    hk::vector<const char *> extensions;

    for (auto &availableExtension : availableExtensions) {
        if (strcmp(availableExtension.extensionName,
                   VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0) {
#ifdef HKDEBUG
            debugUtils = true;
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
        } else if (strcmp(availableExtension.extensionName,
                   VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME) == 0)
        {
            extensions.push_back(VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME);
        }
    }

    extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);

#ifdef HKWINDOWS
    extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#endif

    const char *layers[] = {
        "VK_LAYER_KHRONOS_validation",
    };

    VkInstanceCreateInfo instanceInfo = {};
    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pApplicationInfo = &appInfo;
    instanceInfo.ppEnabledExtensionNames = extensions.data();
    instanceInfo.enabledExtensionCount = extensions.size();
    instanceInfo.ppEnabledLayerNames = layers;
    instanceInfo.enabledLayerCount = sizeof(layers) / sizeof(layers[0]);

    VkDebugUtilsMessengerCreateInfoEXT debugInfo = {};
    if (debugUtils) {
        debugInfo.sType =
            VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debugInfo.messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
        debugInfo.messageType =
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
        debugInfo.pfnUserCallback = debugCallback;

        instanceInfo.pNext = &debugInfo;
    }

    err = vkCreateInstance(&instanceInfo, nullptr, &instance_);
    ALWAYS_ASSERT(!err, "Failed to create Vulkan Instance");

    if (debugUtils) {
        auto vkCreateDebugUtilsMessengerEXT =
            reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(instance_, "vkCreateDebugUtilsMessengerEXT"));

        err = vkCreateDebugUtilsMessengerEXT(instance_, &debugInfo,
                                             nullptr, &debugMessenger);
        ALWAYS_ASSERT(!err, "Failed to create Vulkan Debug Messenger");
    }
}

void VulkanContext::createPhysicalDevice()
{
    u32 deviceCount = 0;
    vkEnumeratePhysicalDevices(instance_, &deviceCount, nullptr);
    ALWAYS_ASSERT(deviceCount, "Failed to find GPUs with Vulkan support");

    hk::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance_, &deviceCount, devices.data());

    LOG_DEBUG("List of all found GPUs:");

    VkPhysicalDeviceFeatures deviceFeatures;
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceMemoryProperties deviceMemoryProperties;

    for (const auto &deviceCur : devices) {
        vkGetPhysicalDeviceFeatures(deviceCur, &deviceFeatures);
        vkGetPhysicalDeviceProperties(deviceCur, &deviceProperties);
        vkGetPhysicalDeviceMemoryProperties(deviceCur, &deviceMemoryProperties);

        LOG_TRACE(deviceProperties.deviceName);

        if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            physicalDevice_ = deviceCur;
            break;
        }
        // LOG_WARN("Non discrete GPUs are not supported by Hikari");
    }

    LOG_DEBUG("Chosen GPU:", deviceProperties.deviceName);

    ALWAYS_ASSERT(physicalDevice_, "Failed to find a suitable GPU");
}

void VulkanContext::createLogicalDevice()
{
    VkResult err;

    hk::QueueFamily graphicsFamily;
    graphicsFamily.findQueue(physicalDevice_, VK_QUEUE_GRAPHICS_BIT);
    hk::QueueFamily computeFamily;
    computeFamily.findQueue(physicalDevice_, VK_QUEUE_COMPUTE_BIT);
    hk::QueueFamily transferFamily;
    transferFamily.findQueue(physicalDevice_, VK_QUEUE_TRANSFER_BIT);

    f32 queuePriority = 1.f;
    hk::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

    if (graphicsFamily) {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = graphicsFamily.index_;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }
    if (computeFamily) {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = computeFamily.index_;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }
    if (transferFamily) {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = transferFamily.index_;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    ALWAYS_ASSERT(queueCreateInfos.size(), "Failed to find at least one queue");

    // Create logical device
    const char *extensionsLogic[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };

    VkPhysicalDeviceFeatures deviceFeatures = {};
    deviceFeatures.samplerAnisotropy = VK_TRUE;

    VkDeviceCreateInfo deviceInfo = {};
    deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceInfo.pQueueCreateInfos = queueCreateInfos.data();
    deviceInfo.queueCreateInfoCount = static_cast<u32>(queueCreateInfos.size());
    deviceInfo.ppEnabledExtensionNames = extensionsLogic;
    deviceInfo.enabledExtensionCount =
        sizeof(extensionsLogic) / sizeof(extensionsLogic[0]);

    deviceInfo.pEnabledFeatures = &deviceFeatures;

    err = vkCreateDevice(physicalDevice_, &deviceInfo, 0, &device_);
    ALWAYS_ASSERT(!err, "Failed to create Vulkan Logical Device");

    // Get queue handles
    graphics_.init(device_, graphicsFamily);
    compute_.init(device_, computeFamily);
    transfer_.init(device_, transferFamily);
}

}

VKAPI_ATTR VkBool32 VKAPI_CALL
debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
              VkDebugUtilsMessageTypeFlagsEXT messageType,
              const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
              void *pUserData)
{
    (void)messageType;
    (void)pUserData;

    switch (messageSeverity) {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: {
        LOG_WARN("Vulkan:", pCallbackData->pMessage);
    } break;

    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: {
        LOG_ERROR("Vulkan:", pCallbackData->pMessage);
    } break;

    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: {
        LOG_INFO("Vulkan:", pCallbackData->pMessage);
    } break;

    default:
        // do nothing
        break;
    }

    // HKBREAK;

    return VK_FALSE;
}
