#include "vkcontext.h"

#include "platform/platform.h"
#include "vulkan/vulkan_win32.h"

#include "vkdebug.h"

#include "utils/spec.h"
#include "utils/settings.h"

namespace hk::vkc {

static struct Context {
    VkInstance instance_ = VK_NULL_HANDLE;
    VkPhysicalDevice adapter_ = VK_NULL_HANDLE;
    VkDevice device_ = VK_NULL_HANDLE;

    Queue graphics_;
    Queue compute_;
    Queue transfer_;

    struct Info {
        InstanceInfo instance;
        DeviceInfo device;

        i32 adapter_index = -1;
        hk::vector<AdapterInfo> adapters;
    } info_;

    /* ===== Config ===== */
    // Instance create info
    string_set required_layers;
    string_set required_inst_exts;
} ctx;

// TEMP START
void submitImmCmd(const std::function<void(VkCommandBuffer cmd)> &&callback)
{
    VkResult err;

    VkCommandBuffer immCommandBuffer_ = ctx.graphics_.createCommandBuffer();
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

    ctx.graphics_.submitAndWait(immCommandBuffer_);

    ctx.graphics_.freeCommandBuffer(immCommandBuffer_);
}
VkInstance       instance() { return ctx.instance_; }
VkPhysicalDevice adapter() { return ctx.adapter_; }
VkDevice         device() { return ctx.device_; }
Queue& graphics() { return ctx.graphics_; }
Queue& compute() { return ctx.compute_; }
Queue& transfer() { return ctx.transfer_; }

const InstanceInfo& instance_info() { return ctx.info_.instance; }
const AdapterInfo& adapter_info(u32 idx) {
    if (!idx) { idx = ctx.info_.adapter_index; }
    return ctx.info_.adapters.at(idx);
}
const DeviceInfo& device_info() { return ctx.info_.device; }
const u32 adapter_count() { return ctx.info_.adapters.size(); }
// TEMP END

void update_instance_info();
void create_instance();

void update_adapter_info();
void pick_adapter();

void create_device();

void init()
{
    LOG_INFO("Initializing Vulkan Context");

    // TODO: move to renderer
    // FIX: update instance info should be called before setting layers and exts
    // because constraints are set in update call
    string_set exts;
    exts.insert(VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME);
    exts.insert(VK_KHR_SURFACE_EXTENSION_NAME);
#ifdef HKWINDOWS
    exts.insert(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#endif
#ifdef HKDEBUG
    exts.insert(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    exts.insert(VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME);
#endif

    string_set layers;
    layers.insert("VK_LAYER_KHRONOS_validation");

    hk::set_setting(Setting::VK_INST_EXTENSIONS, exts);
    hk::set_setting(Setting::VK_LAYERS, layers);

    ctx.required_layers =
        std::get<string_set>(get_setting(Setting::VK_LAYERS));
    ctx.required_inst_exts =
        std::get<string_set>(get_setting(Setting::VK_INST_EXTENSIONS));

    update_instance_info();
    create_instance();

    update_adapter_info();
    pick_adapter();

    create_device();
}

void deinit()
{
    ctx.transfer_.deinit();
    ctx.compute_.deinit();
    ctx.graphics_.deinit();

    if (ctx.device_) {
        vkDestroyDevice(ctx.device_, nullptr);
        ctx.device_ = VK_NULL_HANDLE;
    }

    // hk::debug::deinit(ctx.instance_);

    if (ctx.instance_) {
        vkDestroyInstance(ctx.instance_, nullptr);
        ctx.instance_ = VK_NULL_HANDLE;
    }
}

void update_instance_info()
{
    InstanceInfo &info = ctx.info_.instance;

    // Get Vulkan API version
    vkEnumerateInstanceVersion(&info.api);

    // Get available extensions
    u32 ext_count = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &ext_count, nullptr);
    hk::vector<VkExtensionProperties> exts(ext_count);
    vkEnumerateInstanceExtensionProperties(nullptr, &ext_count, exts.data());

    // Get available layers
    u32 layer_count = 0;
    vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
    hk::vector<VkLayerProperties> layers(layer_count);
    vkEnumerateInstanceLayerProperties(&layer_count, layers.data());

    string_set available_exts;
    for (auto &ext : exts) {
        available_exts.insert(ext.extensionName);
    }
    hk::set_constraints(Setting::VK_INST_EXTENSIONS, available_exts);

    string_set available_layers;
    for (auto &layer : layers) {
        available_exts.insert(layer.layerName);
    }
    hk::set_constraints(Setting::VK_LAYERS, available_layers);

    for (auto &ext : info.exts) {
        auto idx = ctx.required_inst_exts.find(ext.second.extensionName);

        if (idx != ctx.required_inst_exts.end()) {
            ext.first = true;
        }
    }

    for (auto &layer : info.layers) {
        auto idx = ctx.required_layers.find(layer.second.layerName);

        if (idx != ctx.required_layers.end()) {
            layer.first = true;
        }
    }
}

void update_adapter_info()
{
    u32 adapter_count = 0;
    vkEnumeratePhysicalDevices(ctx.instance_, &adapter_count, nullptr);
    ALWAYS_ASSERT(adapter_count, "Failed to find GPUs with Vulkan support");

    hk::vector<VkPhysicalDevice> adapters(adapter_count);
    vkEnumeratePhysicalDevices(ctx.instance_, &adapter_count, adapters.data());

    ctx.info_.adapters.resize(adapter_count);

    for (u32 i = 0; i < adapters.size(); ++i) {
        AdapterInfo &info = ctx.info_.adapters.at(i);
        const VkPhysicalDevice &adapter = adapters.at(i);

        info.adapter = adapter;

        info.all_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        info.features.v11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
        info.features.v12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
        info.features.v13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;

        info.all_features.pNext = &info.features.v11;
        info.features.v11.pNext = &info.features.v12;
        info.features.v12.pNext = &info.features.v13;
        // info.features.v13.pNext = extensions

        vkGetPhysicalDeviceFeatures2(adapter, &info.all_features);

        vkGetPhysicalDeviceProperties(adapter, &info.properties);
        vkGetPhysicalDeviceMemoryProperties(adapter, &info.memory_properties);

        u32 qfamily_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(adapter, &qfamily_count, nullptr);
        info.families.resize(qfamily_count);
        vkGetPhysicalDeviceQueueFamilyProperties(adapter, &qfamily_count, info.families.data());

        u32 ext_count = 0;
        vkEnumerateDeviceExtensionProperties(adapter, 0,  &ext_count, nullptr);
        info.exts.resize(ext_count);
        vkEnumerateDeviceExtensionProperties(adapter, 0,  &ext_count, info.exts.data());
    }
}

void create_instance()
{
    VkResult err;

    InstanceInfo &info = ctx.info_.instance;

    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    // appInfo.pApplicationName = "Editor";
    app_info.pEngineName = "Hikai";
    app_info.apiVersion = info.api;

    hk::vector<const char *> exts;
    hk::vector<const char *> layers;

    b8 is_debug = false;

    for (const auto &ext : ctx.required_inst_exts) {
        exts.push_back(ext);
        if (!strcmp(ext, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) ||
            !strcmp(ext, VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME)) {
            is_debug = true;
        }
    }

    for (const auto &layer : ctx.required_layers) {
        layers.push_back(layer);
    }

    VkInstanceCreateInfo instance_info = {};
    instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_info.pApplicationInfo = &app_info;
    instance_info.ppEnabledExtensionNames = exts.data();
    instance_info.enabledExtensionCount = exts.size();
    instance_info.ppEnabledLayerNames = layers.data();
    instance_info.enabledLayerCount = layers.size();
    instance_info.pNext = is_debug ? &hk::debug::info() : nullptr;

    err = vkCreateInstance(&instance_info, nullptr, &ctx.instance_);
    ALWAYS_ASSERT(!err, "Failed to create Vulkan Instance");

    if (is_debug) {
        hk::debug::init(ctx.instance_);
    }
}

void pick_adapter()
{
    // TODO: improve device picking
    for (u32 i = 0; i < ctx.info_.adapters.size(); ++i) {
        if (ctx.info_.adapters.at(i).properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            ctx.info_.adapter_index = i;
        }
    }

    ALWAYS_ASSERT(ctx.info_.adapter_index != -1, "Failed to find a suitable GPU");
    ctx.adapter_ = ctx.info_.adapters.at(ctx.info_.adapter_index).adapter;
}

void create_device()
{
    VkResult err;

    auto &info = ctx.info_.device;

    info.graphics_family.findQueue(ctx.adapter_, VK_QUEUE_GRAPHICS_BIT);
    info.transfer_family.findQueue(ctx.adapter_, VK_QUEUE_TRANSFER_BIT);
    info.compute_family.findQueue(ctx.adapter_, VK_QUEUE_COMPUTE_BIT);

    f32 queuePriority = 1.f;
    hk::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

    if (info.graphics_family) {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = info.graphics_family.index_;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }
    if (info.compute_family) {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = info.compute_family.index_;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }
    if (info.transfer_family) {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = info.transfer_family.index_;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    ALWAYS_ASSERT(queueCreateInfos.size(), "Failed to find at least one queue");

    // Create logical device
    hk::vector<const char *> extensionsLogic = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };

    VkDeviceCreateInfo device_info = {};
    device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_info.pQueueCreateInfos = queueCreateInfos.data();
    device_info.queueCreateInfoCount = static_cast<u32>(queueCreateInfos.size());
    device_info.ppEnabledExtensionNames = extensionsLogic.data();
    device_info.enabledExtensionCount = extensionsLogic.size();

    // FIX: shoud be configurable, right now just enables ALL supported features
    device_info.pNext = &adapter_info(ctx.info_.adapter_index).all_features;

    err = vkCreateDevice(ctx.adapter_, &device_info, 0, &ctx.device_);
    ALWAYS_ASSERT(!err, "Failed to create Vulkan Logical Device");

    hk::debug::setDevice(ctx.device_);

    hk::debug::setName(ctx.instance_, "Instance");
    hk::debug::setName(ctx.adapter_, "Adapter");
    hk::debug::setName(ctx.device_, "Device");

    // Get queue handles
    ctx.graphics_.init(ctx.device_, ctx.info_.device.graphics_family);
    ctx.compute_.init(ctx.device_, ctx.info_.device.compute_family);
    ctx.transfer_.init(ctx.device_, ctx.info_.device.transfer_family);

    hk::debug::setName(ctx.graphics_.handle(), "Queue - Graphics");
    hk::debug::setName(ctx.compute_.handle(), "Queue - Computer");
    hk::debug::setName(ctx.transfer_.handle(), "Queue - Transfer");
}

}

// FIX: find other place for this
namespace hk::spec {

static hk::vector <AdapterSpec> specs;

HKAPI const AdapterSpec& adapter(u32 idx) { return specs.at(idx); }

void update_adapter_specs()
{
    specs.resize(vkc::ctx.info_.adapters.size());

    hk::vkc::InstanceInfo instance = vkc::ctx.info_.instance;

    AdapterVendor vendor;
    AdapterType type;
    for (u32 i = 0; i < vkc::ctx.info_.adapters.size(); ++i) {
        auto adapter = vkc::ctx.info_.adapters.at(i);
        auto &spec = specs.at(i);

        switch (adapter.properties.vendorID) {
        // Not gonna write all that: https://pcisig.com/membership/member-companies
        case 0x1002: { vendor = AdapterVendor::AMD; } break;
        case 0x10DE: { vendor = AdapterVendor::NVIDIA; } break;
        case 0x13B5: { vendor = AdapterVendor::ARM; } break;
        case 0x5143: { vendor = AdapterVendor::QUALCOMM; } break;
        case 0x8086: { vendor = AdapterVendor::INTEL; } break;

        default: { vendor = AdapterVendor::OTHER; }
        }

        switch(adapter.properties.deviceType) {
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: { type = AdapterType::INTEGRATED; } break;
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: { type = AdapterType::DISCRETE; } break;
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: { type = AdapterType::VIRTUAL; } break;
        case VK_PHYSICAL_DEVICE_TYPE_CPU: { type = AdapterType::CPU; } break;
        case VK_PHYSICAL_DEVICE_TYPE_OTHER: { type = AdapterType::OTHER; } break;
        case VK_PHYSICAL_DEVICE_TYPE_MAX_ENUM: { type = AdapterType::MAX_ADAPTER_TYPE; } break;
        }

        spec.name = adapter.properties.deviceName;
        spec.vendor = vendor;
        spec.type = type;

        spec.api.type = BackendType::VULKAN;

        spec.api.version += std::to_string(VK_API_VERSION_MAJOR(instance.api));
        spec.api.version += '.';
        spec.api.version += std::to_string(VK_API_VERSION_MINOR(instance.api));
        spec.api.version += '.';
        spec.api.version += std::to_string(VK_API_VERSION_PATCH(instance.api));

        /* Spec:
         * The encoding of driverVersion is implementation-defined.
         * It may not use the same encoding as apiVersion. */
        u32 driver = adapter.properties.driverVersion;

        // https://github.com/SaschaWillems/vulkan.gpuinfo.org/blob/master/includes/functions.php#L414
        switch (vendor) {
        case AdapterVendor::NVIDIA: {
            spec.api.driver_version += std::to_string((driver >> 22) & 0x3ff);
            spec.api.driver_version += '.';
            spec.api.driver_version += std::to_string((driver >> 14) & 0x0ff);
            spec.api.driver_version += '.';
            spec.api.driver_version += std::to_string((driver >> 6) & 0x0ff);
            spec.api.driver_version += '.';
            spec.api.driver_version += std::to_string((driver) & 0x003f);
        } break;

        case AdapterVendor::INTEL: {
            spec.api.driver_version += std::to_string(driver >> 14);
            spec.api.driver_version += '.';
            spec.api.driver_version += std::to_string(driver & 0x3fff);
        } break;

        default: {
            // Use Vulkan version conventions if vendor mapping is not available
            spec.api.driver_version += std::to_string(VK_API_VERSION_MAJOR(driver));
            spec.api.driver_version += '.';
            spec.api.driver_version += std::to_string(VK_API_VERSION_MINOR(driver));
            spec.api.driver_version += '.';
            spec.api.driver_version += std::to_string(VK_API_VERSION_PATCH(driver));
        }
        }
    }
}

}

