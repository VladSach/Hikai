#include "BackendVulkan.h"

#include "platform/platform.h"
#include "vendor/vulkan/vulkan.h"
#include "vendor/vulkan/vulkan_win32.h"

static VKAPI_ATTR VkBool32 VKAPI_CALL vkDebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
    (void)messageType; (void)pUserData;

    switch (messageSeverity) {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: {
        LOG_WARN("Vulkan: ", pCallbackData->pMessage);
    } break;

    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: {
        LOG_ERROR("Vulkan: ", pCallbackData->pMessage);
    } break;

    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: {
        LOG_INFO("Vulkan: ", pCallbackData->pMessage);
    } break;

    default:
        LOG_INFO("Vulkan: ", pCallbackData->pMessage);
    }

    return VK_FALSE;
}

void BackendVulkan::init()
{
    LOG_INFO("Initializing Vulkan Backend");

    VkResult err;

    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Blight"; // TODO: make configurable
    appInfo.pEngineName = "Hikai Engine";

    const char *extensions[] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
#ifdef _WIN32
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#endif

#ifdef HKDEBUG
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
    };

    const char *layers[] = {
        "VK_LAYER_KHRONOS_validation",
    };

    // TODO: add layers validation

    VkInstanceCreateInfo instanceInfo = {};
    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pApplicationInfo = &appInfo;
    instanceInfo.ppEnabledExtensionNames = extensions;
    instanceInfo.enabledExtensionCount = sizeof(extensions) /
                                         sizeof(extensions[0]);
    instanceInfo.ppEnabledLayerNames = layers;
    instanceInfo.enabledLayerCount = sizeof(layers)/sizeof(layers[0]);

    err = vkCreateInstance(&instanceInfo, nullptr, &instance);
    ALWAYS_ASSERT(!err, "Failed to create Vulkan Instance");

#ifdef HKDEBUG
    VkDebugUtilsMessengerCreateInfoEXT debugInfo = {};
    debugInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugInfo.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
    debugInfo.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_FLAG_BITS_MAX_ENUM_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
    debugInfo.pfnUserCallback = vkDebugCallback;

    auto vkCreateDebugUtilsMessengerEXT =
        reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));

    vkCreateDebugUtilsMessengerEXT(instance, &debugInfo,
                                   nullptr, &debugMessenger);
#endif

    // Pick physical device
    u32 deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    ALWAYS_ASSERT(deviceCount, "Failed to find GPUs with Vulkan");

    hk::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    physicalDevice = devices[0];
}

void BackendVulkan::deinit()
{
#ifdef HKDEBUG
    auto vkDestroyDebugUtilsMessengerEXT =
        reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
    vkDestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
#endif

    vkDestroyInstance(instance, nullptr);
}
