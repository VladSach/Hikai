#include "vkdebug.h"

#include "defines.h"
#include "core/events.h"

#define HK_VK_GET_PROC_ADDR(name, instance) \
    reinterpret_cast<PFN_##name> \
    (vkGetInstanceProcAddr(instance, #name))

namespace hk::debug {

static VkDevice device = VK_NULL_HANDLE;
static VkDebugUtilsMessengerEXT messenger = VK_NULL_HANDLE;

static PFN_vkCreateDebugUtilsMessengerEXT  vkCreateDebugUtilsMessengerEXT  = nullptr;
static PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT = nullptr;

static PFN_vkCmdBeginDebugUtilsLabelEXT    vkCmdBeginDebugUtilsLabelEXT  = nullptr;
static PFN_vkCmdEndDebugUtilsLabelEXT      vkCmdEndDebugUtilsLabelEXT    = nullptr;
static PFN_vkCmdInsertDebugUtilsLabelEXT   vkCmdInsertDebugUtilsLabelEXT = nullptr;

static PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT = nullptr;
static PFN_vkSetDebugUtilsObjectTagEXT  vkSetDebugUtilsObjectTagEXT  = nullptr;


VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
    void *pUserData);

VkDebugUtilsMessengerCreateInfoEXT& info()
{
    static VkDebugUtilsMessengerCreateInfoEXT out = {};
    out.sType =
        VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    out.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
    out.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
    out.pfnUserCallback = debugCallback;

    return out;
}

void init(VkInstance instance)
{
    VkResult err;

    vkCreateDebugUtilsMessengerEXT =
        HK_VK_GET_PROC_ADDR(vkCreateDebugUtilsMessengerEXT, instance);

    err = vkCreateDebugUtilsMessengerEXT(instance, &info(), nullptr, &messenger);
    ALWAYS_ASSERT(!err, "Failed to create Vulkan Debug Messenger");

    vkDestroyDebugUtilsMessengerEXT =
        HK_VK_GET_PROC_ADDR(vkDestroyDebugUtilsMessengerEXT, instance);

    vkCmdBeginDebugUtilsLabelEXT =
        HK_VK_GET_PROC_ADDR(vkCmdBeginDebugUtilsLabelEXT, instance);
    vkCmdEndDebugUtilsLabelEXT =
        HK_VK_GET_PROC_ADDR(vkCmdEndDebugUtilsLabelEXT, instance);
    vkCmdInsertDebugUtilsLabelEXT =
        HK_VK_GET_PROC_ADDR(vkCmdInsertDebugUtilsLabelEXT, instance);

    vkSetDebugUtilsObjectNameEXT =
        HK_VK_GET_PROC_ADDR(vkSetDebugUtilsObjectNameEXT, instance);
    vkSetDebugUtilsObjectTagEXT =
        HK_VK_GET_PROC_ADDR(vkSetDebugUtilsObjectTagEXT, instance);

    LOG_DEBUG("Vulkan Debugger created");
}

void deinit(VkInstance instance)
{
    if (!messenger) return;

    vkDestroyDebugUtilsMessengerEXT(instance, messenger, nullptr);
    messenger = VK_NULL_HANDLE;
}

namespace label {

void begin(VkCommandBuffer cmd, const std::string &name, const hkm::vec4f &color)
{
    if (!vkCmdBeginDebugUtilsLabelEXT) { return; }

    VkDebugUtilsLabelEXT label = {};
    label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
    label.pLabelName = name.c_str();
    label.color[0] = color.x;
    label.color[1] = color.y;
    label.color[2] = color.z;
    label.color[3] = color.w;

    vkCmdBeginDebugUtilsLabelEXT(cmd, &label);
}

void end(VkCommandBuffer cmd)
{
    if (!vkCmdEndDebugUtilsLabelEXT) { return; }

    vkCmdEndDebugUtilsLabelEXT(cmd);
}

void insert(VkCommandBuffer cmd, const std::string &name, const hkm::vec4f &color)
{
    if (!vkCmdInsertDebugUtilsLabelEXT) { return; }

    VkDebugUtilsLabelEXT label = {};
    label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
    label.pLabelName = name.c_str();
    label.color[0] = color.x;
    label.color[1] = color.y;
    label.color[2] = color.z;
    label.color[3] = color.w;

    vkCmdInsertDebugUtilsLabelEXT(cmd, &label);
}

} // namespace label

/*************** Object naming and tagging ***************/
void setDevice(VkDevice dev)
{
    device = dev;
}

inline void setName(VkObjectType type, u64 handle, const std::string &name)
{
    if (!vkSetDebugUtilsObjectNameEXT) { return; }

    VkDebugUtilsObjectNameInfoEXT info = {};
    info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
    info.objectType = type;
    info.objectHandle = handle;
    info.pObjectName = name.c_str();

    vkSetDebugUtilsObjectNameEXT(device, &info);
}

inline void setTag(VkObjectType type, u64 handle, const hk::vector<u32> &data)
{
    if (!vkSetDebugUtilsObjectTagEXT) { return; }

    VkDebugUtilsObjectTagInfoEXT info = {};
    info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_TAG_INFO_EXT;
    info.objectType = type;
    info.objectHandle = handle;
    info.tagName = 0;
    info.tagSize = data.size();
    info.pTag  = data.data();

    vkSetDebugUtilsObjectTagEXT(device, &info);
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
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: {
        LOG_INFO("Vulkan:", pCallbackData->pMessage);
    } break;

    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: {
        LOG_WARN("Vulkan:", pCallbackData->pMessage);
    } break;

    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: {
        LOG_ERROR("Vulkan:", pCallbackData->pMessage);
    } break;

    default:
        // do nothing
        break;
    }

    // TODO: turn on
    // hk::log::dispatch();

    // hk::event::fire(hk::event::ERROR_VK_VALIDATION_LAYER, {});

    return VK_FALSE;
}

void setName(VkInstance instance, const std::string &name)
{
    setName(VK_OBJECT_TYPE_INSTANCE, reinterpret_cast<u64>(instance), name);
}

void setName(VkPhysicalDevice dev, const std::string &name)
{
    setName(VK_OBJECT_TYPE_PHYSICAL_DEVICE, reinterpret_cast<u64>(dev), name);
}

void setName(VkDevice dev, const std::string &name)
{
    setName(VK_OBJECT_TYPE_DEVICE, reinterpret_cast<u64>(dev), name);
}

void setName(VkQueue queue, const std::string &name)
{
    setName(VK_OBJECT_TYPE_QUEUE, reinterpret_cast<u64>(queue), name);
}

void setName(VkSemaphore semaphore, const std::string &name)
{
    setName(VK_OBJECT_TYPE_SEMAPHORE, reinterpret_cast<u64>(semaphore), name);
}

void setName(VkCommandBuffer cmd, const std::string &name)
{
    setName(VK_OBJECT_TYPE_COMMAND_BUFFER, reinterpret_cast<u64>(cmd), name);
}

void setName(VkFence fence, const std::string &name)
{
    setName(VK_OBJECT_TYPE_FENCE, reinterpret_cast<u64>(fence), name);
}

void setName(VkDeviceMemory memory, const std::string &name)
{
    setName(VK_OBJECT_TYPE_DEVICE_MEMORY, reinterpret_cast<u64>(memory), name);
}

void setName(VkBuffer buf, const std::string &name)
{
    setName(VK_OBJECT_TYPE_BUFFER, reinterpret_cast<u64>(buf), name);
}

void setName(VkImage image, const std::string &name)
{
    setName(VK_OBJECT_TYPE_IMAGE, reinterpret_cast<u64>(image), name);
}

void setName(VkEvent event, const std::string &name)
{
    setName(VK_OBJECT_TYPE_EVENT, reinterpret_cast<u64>(event), name);
}

void setName(VkQueryPool pool, const std::string &name)
{
    setName(VK_OBJECT_TYPE_QUERY_POOL, reinterpret_cast<u64>(pool), name);
}

void setName(VkBufferView view, const std::string &name)
{
    setName(VK_OBJECT_TYPE_BUFFER_VIEW, reinterpret_cast<u64>(view), name);
}

void setName(VkImageView view, const std::string &name)
{
    setName(VK_OBJECT_TYPE_IMAGE_VIEW, reinterpret_cast<u64>(view), name);
}

void setName(VkShaderModule shader, const std::string &name)
{
    setName(VK_OBJECT_TYPE_SHADER_MODULE, reinterpret_cast<u64>(shader), name);
}

void setName(VkPipelineCache cache, const std::string &name)
{
    setName(VK_OBJECT_TYPE_PIPELINE_CACHE, reinterpret_cast<u64>(cache), name);
}

void setName(VkPipelineLayout layout, const std::string &name)
{
    setName(VK_OBJECT_TYPE_PIPELINE_LAYOUT, reinterpret_cast<u64>(layout), name);
}

void setName(VkRenderPass pass, const std::string &name)
{
    setName(VK_OBJECT_TYPE_RENDER_PASS, reinterpret_cast<u64>(pass), name);
}

void setName(VkPipeline pipeline, const std::string &name)
{
    setName(VK_OBJECT_TYPE_PIPELINE, reinterpret_cast<u64>(pipeline), name);
}

void setName(VkDescriptorSetLayout layout, const std::string &name)
{
    setName(VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT,
            reinterpret_cast<u64>(layout), name);
}

void setName(VkSampler sampler, const std::string &name)
{
    setName(VK_OBJECT_TYPE_SAMPLER, reinterpret_cast<u64>(sampler), name);
}

void setName(VkDescriptorPool pool, const std::string &name)
{
    setName(VK_OBJECT_TYPE_DESCRIPTOR_POOL, reinterpret_cast<u64>(pool), name);
}

void setName(VkDescriptorSet set, const std::string &name)
{
    setName(VK_OBJECT_TYPE_DESCRIPTOR_SET, reinterpret_cast<u64>(set), name);
}

void setName(VkFramebuffer framebuf, const std::string &name)
{
    setName(VK_OBJECT_TYPE_FRAMEBUFFER, reinterpret_cast<u64>(framebuf), name);
}

void setName(VkCommandPool pool, const std::string &name)
{
    setName(VK_OBJECT_TYPE_COMMAND_POOL, reinterpret_cast<u64>(pool), name);
}

}
