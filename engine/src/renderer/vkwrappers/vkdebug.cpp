#include "vkdebug.h"

#include "core/events.h"

#include "vulkan/vk_enum_string_helper.h"

#include "hkstl/strings/hkstring.h"

#define HK_VK_GET_PROC_ADDR(name, instance) \
    reinterpret_cast<PFN_##name>            \
    (vkGetInstanceProcAddr(instance, #name))

namespace hk::debug {

static VkDevice device = VK_NULL_HANDLE;

/* ===== Function Pointer ==== */
static VkDebugUtilsMessengerEXT messenger = VK_NULL_HANDLE;

static PFN_vkCreateDebugUtilsMessengerEXT  vkCreateDebugUtilsMessengerEXT  = nullptr;
static PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT = nullptr;

static PFN_vkCmdBeginDebugUtilsLabelEXT    vkCmdBeginDebugUtilsLabelEXT  = nullptr;
static PFN_vkCmdEndDebugUtilsLabelEXT      vkCmdEndDebugUtilsLabelEXT    = nullptr;
static PFN_vkCmdInsertDebugUtilsLabelEXT   vkCmdInsertDebugUtilsLabelEXT = nullptr;

static PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT = nullptr;
static PFN_vkSetDebugUtilsObjectTagEXT  vkSetDebugUtilsObjectTagEXT  = nullptr;



VKAPI_ATTR VkBool32 VKAPI_CALL validation_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
    void *user_data);

struct ValidationMessage {
    enum Severity : u8 {
        VERBOSE, // diagnostic messages from the loader, layers, and drivers
        INFO,    // resource details that may be handy when debugging
        WARNING, // issues that don't violate spec but MAY expose bug in app
        ERROR    // violations of the Vulkan specification
    } severity;

    enum Type : u8 {
        GENERAL,    // non-specification, non-performance event
        VALIDATION, // specification violations or potential mistakes
        PERFORMANCE // potentially non-optimal use of Vulkan
    } type;

    /* ID is not really useful for me
     * if validation, then related to the internal number associated with the message being triggered
     * otherwise: ??? */
    i32 id;

    /* Identifies the particular message ID, can be NULL or UTF8
     * if validation, then contains the portion of the Vulkan specification
     * otherwise ??? */
    std::string vuid;

    /* 0th element is always guaranteed to be the most important object for the message */
    hk::vector<VkDebugUtilsObjectNameInfoEXT> objects;

    /* ===== Parsed Message Components ===== */
    std::string message;
    std::string vk_spec; // Spec reference
    std::string lunarg_url; // Url to section of the spec
};

struct ValidationContext {
    hk::vector<ValidationMessage> muted_messages;
    hk::vector<ValidationMessage> messages;

    // config
    // TODO: change to WARNING, when amd update their layer
    ValidationMessage::Severity threshold = ValidationMessage::ERROR;
};

static ValidationContext ctx;

VkDebugUtilsMessengerCreateInfoEXT& info()
{
    static VkDebugUtilsMessengerCreateInfoEXT out = {};

    out.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;

    // out.flags = 0; // Reserved for future use
    out.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT    |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    out.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT     |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT  |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        // VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT;

    out.pfnUserCallback = validation_callback;

    /* Notes
     * 1. Debug Printf functionality and GPU-Assisted validation
     * cannot be run at the same time
     * 2. Most Khronos Validation layer features can be used simultaneously.
     * However, this could result in noticeable performance degradation.
     * The best practice is to run Core validation, GPU-Assisted validation,
     * Synchronization Validation and Best practices validation features
     * individually.
     */
    static hk::vector<VkValidationFeatureEnableEXT> features = {
        // VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT,
        // VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT,
        // VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT,
        // VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT,
    };

    static VkValidationFeaturesEXT validation_features = {};
    validation_features.sType = { VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT };
    validation_features.enabledValidationFeatureCount = features.size();
    validation_features.pEnabledValidationFeatures = features.data();

    out.pNext = &validation_features;

    return out;
}

void init(VkInstance instance)
{
    VkResult err;

    vkCreateDebugUtilsMessengerEXT = HK_VK_GET_PROC_ADDR(vkCreateDebugUtilsMessengerEXT, instance);

    err = vkCreateDebugUtilsMessengerEXT(instance, &info(), nullptr, &messenger);
    ALWAYS_ASSERT(!err, "Failed to create Vulkan Debug Messenger");

    vkDestroyDebugUtilsMessengerEXT = HK_VK_GET_PROC_ADDR(vkDestroyDebugUtilsMessengerEXT, instance);

    vkCmdBeginDebugUtilsLabelEXT  = HK_VK_GET_PROC_ADDR(vkCmdBeginDebugUtilsLabelEXT, instance);
    vkCmdEndDebugUtilsLabelEXT    = HK_VK_GET_PROC_ADDR(vkCmdEndDebugUtilsLabelEXT, instance);
    vkCmdInsertDebugUtilsLabelEXT = HK_VK_GET_PROC_ADDR(vkCmdInsertDebugUtilsLabelEXT, instance);

    vkSetDebugUtilsObjectNameEXT = HK_VK_GET_PROC_ADDR(vkSetDebugUtilsObjectNameEXT, instance);
    vkSetDebugUtilsObjectTagEXT  = HK_VK_GET_PROC_ADDR(vkSetDebugUtilsObjectTagEXT, instance);

    // TODO:
    // if panic mode => call panic
    // if not => add message to struct with messages to print them later

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

/* ===== Object naming and tagging ===== */
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

/* ===== Debug Callback ===== */
VKAPI_ATTR VkBool32 VKAPI_CALL validation_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
    void *user_data)
{
    (void)user_data;

    ValidationMessage msg;

    /* ===== Extract Callback Info ===== */

    // HACK: Not gonna put this long enum values here
    switch (severity) {
    case 1:    { msg.severity = ValidationMessage::VERBOSE; } break;
    case 16:   { msg.severity = ValidationMessage::INFO;    } break;
    case 256:  { msg.severity = ValidationMessage::WARNING; } break;
    case 4096: { msg.severity = ValidationMessage::ERROR;   } break;
    default: break;
    }

    switch(type) {
    case 1: { msg.type = ValidationMessage::GENERAL;     } break;
    case 2: { msg.type = ValidationMessage::VALIDATION;  } break;
    case 4: { msg.type = ValidationMessage::PERFORMANCE; } break;
    default: break;
    }

    msg.id   = callback_data->messageIdNumber;
    msg.vuid = callback_data->pMessageIdName ? callback_data->pMessageIdName : "(NULL)";
    std::string message = callback_data->pMessage ? callback_data->pMessage : "(NULL)";

    u32 object_count = callback_data->objectCount;
    msg.objects.assign(callback_data->pObjects, callback_data->pObjects + object_count);

    /* ===== Extract Message Components ===== */

    // Strip and save url
    u64 url_pos = message.find("https://");
    if (url_pos != std::string::npos) {
        msg.lunarg_url = message.substr(url_pos);
        msg.lunarg_url.pop_back(); // remove trailing ')'
        message = message.substr(0, url_pos - 2); // remove " (" + url
    }

    // Strip and save spec reference
    u64 spec_pos = message.find("The Vulkan spec states:");
    if (spec_pos != std::string::npos) {
        msg.vk_spec = message.substr(spec_pos + 24);
        msg.vk_spec += '.';
        message = message.substr(0, spec_pos);
        message.pop_back(); // remove '\n'
    }

    msg.message = message;

    /* ===== Mute if below threshold ===== */
    if (msg.severity < ctx.threshold) {
        ctx.muted_messages.push_back(msg);
        return VK_FALSE;
    }

    ctx.messages.push_back(msg);

    /* ===== Format Message Based on Available Info ===== */

    constexpr const char *severity_lookup[] = {
        "Verbose",
        "Info",
        "Warning",
        "Error",
    };
    constexpr const char *type_lookup[] = {
        "General",
        "Validation",
        "Performance",
    };

    std::string title;
    title += type_lookup[msg.type];
    title += ' ';
    title += severity_lookup[msg.severity];

    std::string header = msg.vuid + '(' + std::to_string(msg.id) + ')';

    std::string formatted_message;

    formatted_message += msg.message + '\n';

    /* vuid_parts usage:
     *           [0] |         [1]         |     [2]     |  [3]
     * Explicit: VUID-vkResetDescriptorPool-descriptorPool-00313
     * Implicit: VUID-vkBindBufferMemory-memory-parameter */
    hk::vector<std::string> vuid_parts;
    vuid_parts = hk::strtok(msg.vuid, "-");

    std::string vkdoc_url;
    if (vuid_parts.size()) {
        vkdoc_url = "https://vkdoc.net/man/" + vuid_parts[1] + '#' + msg.vuid;
    }

    /* Object Info
     *
     * objectType
     * u64 objectHandle
     * pObjectName - either NULL or a null-terminated UTF-8 string */
    std::string objects_info = "Objects: " + std::to_string(object_count);
    for (u32 i = 0; i < object_count; ++i) {
        auto &object = msg.objects.at(i);

        // Example: [0] 0x56313fd28a00, type: 6, name: NULL
        objects_info += "\n    [" + std::to_string(i) + "]" + " ";

        // objects_info += std::to_string(object.objectHandle) + " "; // FIX: convert handel to hex

        objects_info += "type: ";
        objects_info += string_VkObjectType(object.objectType);
        objects_info += " ";

        objects_info += "name: ";
        objects_info += object.pObjectName ? object.pObjectName : "NULL";
    }

    if (object_count) {
        formatted_message += '\n' + objects_info + '\n';
    }

    if (!msg.vk_spec.empty()) {
        formatted_message +=
            "\nSpecification Reference:\n" + msg.vk_spec + "\n\n";
    }

    if (!msg.lunarg_url.empty()) {
        formatted_message +=
            "<a href=\"" + msg.lunarg_url + "\">LunarG</a>" + ", " +
            "<a href=\"" + vkdoc_url + "\">VulkanHub</a>" + '\n';
    }

    LOG_ERROR(msg.message);
    PANIC(title, msg.vuid, formatted_message);

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
