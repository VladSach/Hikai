#include "RenderDevice.h"

#include "platform/platform.h"
#include "renderer/ShaderManager.h"
#include "renderer/VertexLayout.h"

#ifdef HKWINDOWS
#include "vendor/vulkan/vulkan_win32.h"
#endif

// TODO: change with hikai implementation
#include <algorithm>
#include <set>

// FIX: temp
#include "renderer/UBManager.h"
#include "object/Model.h"
#include "utils/loaders/ModelLoader.h"
static hk::Model *model;

#include "utils/loaders/ImageLoader.h"
static hk::Image *texture;

namespace hk {

RenderDevice *device()
{
    static RenderDevice *globalDevice = nullptr;

    if (globalDevice == nullptr) {
        globalDevice = new RenderDevice();
    }
    return globalDevice;
}

}

static VKAPI_ATTR VkBool32 VKAPI_CALL vkDebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
    void *pUserData);

void RenderDevice::init(const Window *window)
{
    LOG_INFO("Initializing Vulkan Render Device");

    window_ = window;

    createInstance();
    createSurface();
    createPhysicalDevice();
    createLogicalDevice();

    createSyncObjects();

    createCommandPool();
    createCommandBuffer();

    createSwapchain();

    createDepthResources();

    createImageViews();

    createRenderPass();

    hk::ubo::init();

    createGraphicsPipeline();
    createFramebuffers();


    model = hk::loader::loadModel("assets/models/smooth_vase.obj");
    model->populateBuffers();

    texture =
        hk::loader::loadImage("assets/textures/prototype/PNG/Dark/texture_01.png");
}

void RenderDevice::cleanupSwapchain()
{
    vkDeviceWaitIdle(device);

    for (auto framebuffer : scFramebuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }

    for (auto imageView : scImageViews) {
        vkDestroyImageView(device, imageView, nullptr);
    }

    vkDestroySwapchainKHR(device, swapchain, nullptr);
}

void RenderDevice::deinit()
{
    cleanupSwapchain();

    model->deinit();

    depthImage.deinit();

    vkDestroyPipeline(device, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);

    vkDestroyRenderPass(device, renderPass, nullptr);

    hk::ubo::deinit();

    vkDestroySemaphore(device, acquireSemaphore, nullptr);
    vkDestroySemaphore(device, submitSemaphore, nullptr);
    vkDestroyFence(device, inFlightFence, nullptr);
    vkDestroyFence(device, immCommandFence_, nullptr);

    vkDestroyCommandPool(device, immCommandPool_, nullptr);
    vkDestroyCommandPool(device, commandPool, nullptr);

    vkDestroyDevice(device, nullptr);

    if (debugUtils) {
        auto vkDestroyDebugUtilsMessengerEXT =
            reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
        vkDestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    }

    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
}

void RenderDevice::draw()
{
    VkResult err;

    vkWaitForFences(device, 1, &inFlightFence, VK_TRUE, UINT64_MAX);

    u32 imageIndex;
    err = vkAcquireNextImageKHR(device, swapchain, 0,
                                acquireSemaphore, 0, &imageIndex);

    if (err == VK_ERROR_OUT_OF_DATE_KHR) {
        cleanupSwapchain();
        createSwapchain();
        createImageViews();
        createDepthResources();
        createFramebuffers();
        return;
    }

    vkResetFences(device, 1, &inFlightFence);
    vkResetCommandBuffer(commandBuffer, 0);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    err = vkBeginCommandBuffer(commandBuffer, &beginInfo);
    ALWAYS_ASSERT(!err, "Failed to begin Command Buffer");

    VkClearValue clearValue[2] = {};
    clearValue[0].color = { 0.f, 0.f, 0.f, 1.f };
    clearValue[1].depthStencil = { 0.f, 0 };

    VkRenderPassBeginInfo rpBeginInfo = {};
    rpBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpBeginInfo.renderPass = renderPass;
    rpBeginInfo.renderArea.offset = { 0, 0 };
    rpBeginInfo.renderArea.extent = scExtent;
    rpBeginInfo.framebuffer = scFramebuffers[imageIndex];
    rpBeginInfo.clearValueCount = 2;
    rpBeginInfo.pClearValues = clearValue;

    vkCmdBeginRenderPass(commandBuffer, &rpBeginInfo,
                         VK_SUBPASS_CONTENTS_INLINE);
    {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          graphicsPipeline);

        VkViewport viewport = {};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(scExtent.width);
        viewport.height = static_cast<float>(scExtent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor = {};
        scissor.offset = {0, 0};
        scissor.extent = scExtent;
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        model->bind(commandBuffer);
        texture->bind();
        getGlobalDescriptorWriter()->updateSet(*getGlobalDescriptorSet());

        vkCmdBindDescriptorSets(commandBuffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                pipelineLayout, 0, 1,
                                getGlobalDescriptorSet(), 0, nullptr);

        vkCmdDrawIndexed(commandBuffer, model->indexCount(), 1, 0, 0, 0);
    }
    vkCmdEndRenderPass(commandBuffer);
    err = vkEndCommandBuffer(commandBuffer);
    ALWAYS_ASSERT(!err, "Failed to end Command Buffer");

    VkPipelineStageFlags waitStage =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    submitInfo.pSignalSemaphores = &submitSemaphore;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &acquireSemaphore;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitDstStageMask = &waitStage;

    err = vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFence);
    ALWAYS_ASSERT(!err, "Failed to submit Vulkan draw Command Buffer");

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pWaitSemaphores = &submitSemaphore;
    presentInfo.waitSemaphoreCount = 1;

    err = vkQueuePresentKHR(presentQueue, &presentInfo);
    if (err == VK_ERROR_OUT_OF_DATE_KHR) {
        cleanupSwapchain();
        createSwapchain();
        createImageViews();
        createDepthResources();
        createFramebuffers();
    }
}

void RenderDevice::createInstance()
{
    VkResult err;

    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Sandbox"; // TODO: make configurable
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
        debugInfo.pfnUserCallback = vkDebugCallback;

        instanceInfo.pNext = &debugInfo;
    }

    err = vkCreateInstance(&instanceInfo, nullptr, &instance);
    ALWAYS_ASSERT(!err, "Failed to create Vulkan Instance");

    if (debugUtils) {
        auto vkCreateDebugUtilsMessengerEXT =
            reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));

        err = vkCreateDebugUtilsMessengerEXT(instance, &debugInfo,
                                             nullptr, &debugMessenger);
        ALWAYS_ASSERT(!err, "Failed to create Vulkan Debug Messenger");
    }
}

void RenderDevice::createSurface()
{
    VkResult err = VK_ERROR_UNKNOWN;

    // TODO: add other platforms
#ifdef HKWINDOWS
    const WinWindow *win = static_cast<const WinWindow *>(window_);
    VkWin32SurfaceCreateInfoKHR surfaceInfo = {};
    surfaceInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surfaceInfo.hwnd = win->getHWnd();
    surfaceInfo.hinstance = win->getHInstance();
    err = vkCreateWin32SurfaceKHR(instance, &surfaceInfo, 0, &surface);
#endif // HKWINDOWS

    ALWAYS_ASSERT(!err, "Failed to create Vulkan Surface");
}

void RenderDevice::createPhysicalDevice()
{
    u32 deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    ALWAYS_ASSERT(deviceCount, "Failed to find GPUs with Vulkan support");

    hk::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

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
            physicalDevice = deviceCur;
            break;
        }
        // LOG_WARN("Non discrete GPUs are not supported by Hikari");
    }

    LOG_DEBUG("Chosen GPU:", deviceProperties.deviceName);

    ALWAYS_ASSERT(physicalDevice, "Failed to find a suitable GPU");
}

void RenderDevice::createLogicalDevice()
{
    VkResult err;

    // Pick queue families
    u32 queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice,
                                             &queueFamilyCount, nullptr);

    hk::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount,
                                             queueFamilies.data());

    VkBool32 presentSupport;
    for (u32 i = 0; i < queueFamilyCount; i++) {
        auto &queueFamily = queueFamilies[i];

        if (queueFamily.queueCount <= 0) {
            continue;
        }

        if (graphicsFamily == VK_QUEUE_FAMILY_IGNORED &&
            queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            graphicsFamily = i;
        }

        if (transferFamily == VK_QUEUE_FAMILY_IGNORED &&
            queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT)
        {
            transferFamily = i;
        }

        if (computeFamily == VK_QUEUE_FAMILY_IGNORED &&
            queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)
        {
            computeFamily = i;
        }

        presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i,
                                             surface, &presentSupport);
        if (presentFamily == VK_QUEUE_FAMILY_IGNORED && presentSupport) {
            presentFamily = i;
        }
    }

    b8 res;
    res = (graphicsFamily != VK_QUEUE_FAMILY_IGNORED);
    ALWAYS_ASSERT(res, "Failed to find graphics queue");
    res = (computeFamily != VK_QUEUE_FAMILY_IGNORED);
    ALWAYS_ASSERT(res, "Failed to find compute queue");
    res = (transferFamily != VK_QUEUE_FAMILY_IGNORED);
    ALWAYS_ASSERT(res, "Failed to find transfer queue");
    res = (presentFamily != VK_QUEUE_FAMILY_IGNORED);
    ALWAYS_ASSERT(res, "Failed to find present queue");

    // Create queue families
    f32 queuePriority = 1.f;
    hk::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<u32> uniqueQueueFamilies = {
        graphicsFamily,
        computeFamily,
        transferFamily,
        presentFamily
    };

    for (u32 queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

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

    err = vkCreateDevice(physicalDevice, &deviceInfo, 0, &device);
    ALWAYS_ASSERT(!err, "Failed to create Vulkan Logical Device");

    // Get queue handles
    vkGetDeviceQueue(device, graphicsFamily, 0, &graphicsQueue);
    vkGetDeviceQueue(device, computeFamily, 0, &computeQueue);
    vkGetDeviceQueue(device, transferFamily, 0, &transferQueue);
    vkGetDeviceQueue(device, presentFamily, 0, &presentQueue);
}

void RenderDevice::createSwapchain()
{
    VkResult err;

    VkSurfaceCapabilitiesKHR surfaceCaps = {};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface,
                                              &surfaceCaps);

    u32 formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface,
                                         &formatCount, nullptr);

    hk::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface,
                                         &formatCount, surfaceFormats.data());

    // TODO: change format
    for (const auto &format : surfaceFormats) {
        if (format.format == VK_FORMAT_B8G8R8A8_UNORM) {
            surfaceFormat = format;
            break;
        }
    }

    u32 presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface,
                                              &presentModeCount, nullptr);

    hk::vector<VkPresentModeKHR> presentModes(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(
            physicalDevice, surface, &presentModeCount, presentModes.data());

    for (const auto &mode : presentModes) {
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            presentMode = mode;
            break;
        }

        // FIFO mode is the one that is always supported
        presentMode = VK_PRESENT_MODE_FIFO_KHR;
    }

    scExtent = { window_->getWidth(), window_->getHeight() };
    scExtent.width = std::clamp(scExtent.width,
                                surfaceCaps.minImageExtent.width,
                                surfaceCaps.maxImageExtent.width);
    scExtent.height = std::clamp(scExtent.height,
                                 surfaceCaps.minImageExtent.height,
                                 surfaceCaps.maxImageExtent.height);

    u32 imageCount = surfaceCaps.minImageCount + 1 < surfaceCaps.maxImageCount
                                             ? surfaceCaps.minImageCount + 1
                                             : surfaceCaps.maxImageCount;

    VkSwapchainCreateInfoKHR swapchainInfo = {};
    swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainInfo.surface = surface;
    swapchainInfo.imageFormat = surfaceFormat.format;
    swapchainInfo.imageColorSpace = surfaceFormat.colorSpace;
    swapchainInfo.preTransform = surfaceCaps.currentTransform;
    swapchainInfo.imageExtent = scExtent;
    swapchainInfo.minImageCount = imageCount;
    swapchainInfo.imageArrayLayers = 1;
    swapchainInfo.presentMode = presentMode;
    swapchainInfo.clipped = VK_TRUE;
    swapchainInfo.imageUsage =
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
        VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    u32 queueFamilyIndices[] = { graphicsFamily, computeFamily,
                                 transferFamily, presentFamily };

    if (graphicsFamily != presentFamily) {
        swapchainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchainInfo.queueFamilyIndexCount = 2;
        swapchainInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    err = vkCreateSwapchainKHR(device, &swapchainInfo, 0, &swapchain);
    ALWAYS_ASSERT(!err, "Failed to create Vulkan Swapchain");

    u32 scImageCount = 0;
    vkGetSwapchainImagesKHR(device, swapchain, &scImageCount, nullptr);

    scImages.resize(scImageCount);
    vkGetSwapchainImagesKHR(device, swapchain, &scImageCount, scImages.data());

    scImageFormat = surfaceFormat.format;
}

void RenderDevice::createImageViews()
{
    VkResult err;

    scImageViews.resize(scImages.size());
    for (u32 i = 0; i < scImages.size(); i++) {
        VkImageViewCreateInfo viewInfo = {};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = scImages[i];
        viewInfo.format = scImageFormat;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;
        viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        err = vkCreateImageView(device, &viewInfo, nullptr, &scImageViews[i]);
        ALWAYS_ASSERT(!err, "Failed to create Vulkan Image View");
    }
}

void RenderDevice::createRenderPass()
{
    VkResult err;

    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = scImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription depthAttachment = {};
    depthAttachment.format = depthImage.format();
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout =
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef = {};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout =
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    VkAttachmentDescription attachments[] = {
        colorAttachment, depthAttachment
    };

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask =
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 2;
    renderPassInfo.pAttachments = attachments;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    err = vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass);
    ALWAYS_ASSERT(!err, "Failed to create Vulkan Render Pass");
}

void RenderDevice::createGraphicsPipeline()
{
    VkResult err;

    std::string path = "assets\\shaders\\";

    hk::dxc::ShaderDesc desc;
    desc.path = path + "VisibleNormalsVS.hlsl";
    desc.entry = "main";
    desc.type = ShaderType::Vertex;
    desc.model = ShaderModel::SM_6_0;
    desc.ir = ShaderIR::SPIRV;
#ifdef HKDEBUG
    desc.debug = true;
#else
    desc.debug = false;
#endif

    auto vertShaderCode = hk::dxc::loadShader(desc);

    desc.path = path + "VisibleNormalsPS.hlsl";
    desc.type = ShaderType::Pixel;
    auto fragShaderCode = hk::dxc::loadShader(desc);

    VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
    vertShaderStageInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
    fragShaderStageInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] =
        { vertShaderStageInfo, fragShaderStageInfo };

    hk::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState = {};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<u32>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkVertexInputBindingDescription bindingDescription = {};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    hk::vector<hk::Format> layout = {
        // position
        hk::Format::SIGNED | hk::Format::FLOAT |
        hk::Format::VEC3 | hk::Format::B32,

        // normal
        hk::Format::SIGNED | hk::Format::FLOAT |
        hk::Format::VEC3 | hk::Format::B32,

        // texture coordinates
        hk::Format::SIGNED | hk::Format::FLOAT |
        hk::Format::VEC2 | hk::Format::B32,
    };

    hk::vector<VkVertexInputAttributeDescription> attributeDescs =
        hk::createVertexLayout(layout);

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = attributeDescs.size();
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescs.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType =
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<f32>(scExtent.width);
    viewport.height = static_cast<f32>(scExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = { 0, 0 };
    scissor.extent = scExtent;

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType =
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType =
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT |
        VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT |
        VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor =
        VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType =
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    hk::vector<VkDescriptorSetLayout> descriptorSetsLayouts = {
        getGlobalDescriptorSetLayout(),
    };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = descriptorSetsLayouts.size();
    pipelineLayoutInfo.pSetLayouts = descriptorSetsLayouts.data();

    err = vkCreatePipelineLayout(device, &pipelineLayoutInfo,
                                 nullptr, &pipelineLayout);
    ALWAYS_ASSERT(!err, "Failed to create Vulkan Pipeline Layout");

    VkPipelineDepthStencilStateCreateInfo depthStencil = {};
    depthStencil.sType =
        VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.minDepthBounds = 0.0f; // Optional
    depthStencil.maxDepthBounds = 1.0f; // Optional
    depthStencil.stencilTestEnable = VK_FALSE;
    depthStencil.front = {}; // Optional
    depthStencil.back = {}; // Optional

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = nullptr;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.pDepthStencilState = &depthStencil;

    err = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1,
                                    &pipelineInfo, nullptr, &graphicsPipeline);
    ALWAYS_ASSERT(!err, "Failed to create Vulkan Graphics Pipeline");

    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    vkDestroyShaderModule(device, vertShaderModule, nullptr);
}

void RenderDevice::createFramebuffers()
{
    VkResult err;

    scFramebuffers.resize(scImageViews.size());
    for (u32 i = 0; i < scImageViews.size(); i++) {
        VkImageView attachments[] = {
            scImageViews[i],
            depthImage.view()
        };

        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = 2;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = scExtent.width;
        framebufferInfo.height = scExtent.height;
        framebufferInfo.layers = 1;

        err = vkCreateFramebuffer(device, &framebufferInfo,
                                  nullptr, &scFramebuffers[i]);
        ALWAYS_ASSERT(!err, "Failed to create Vulkan Framebuffer");
    }
}

void RenderDevice::createCommandPool()
{
    VkResult err;

    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = graphicsFamily;

    err = vkCreateCommandPool(device, &poolInfo, nullptr, &immCommandPool_);
    ALWAYS_ASSERT(!err, "Failed to create Vulkan Command Pool");

    err = vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool);
    ALWAYS_ASSERT(!err, "Failed to create Vulkan Command Pool");
}

void RenderDevice::createCommandBuffer()
{
    VkResult err;

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = immCommandPool_;
    allocInfo.commandBufferCount = 1;

    err = vkAllocateCommandBuffers(device, &allocInfo, &immCommandBuffer_);
    ALWAYS_ASSERT(!err, "Failed to allocate Vulkan Command Buffer");

    err = vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);
    ALWAYS_ASSERT(!err, "Failed to allocate Vulkan Command Buffer");
}

void RenderDevice::createSyncObjects()
{
    VkResult err;

    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    err = vkCreateSemaphore(device, &semaphoreInfo, nullptr, &acquireSemaphore);
    ALWAYS_ASSERT(!err, "Failed to create Vulkan Semaphore");

    err = vkCreateSemaphore(device, &semaphoreInfo, nullptr, &submitSemaphore);
    ALWAYS_ASSERT(!err, "Failed to create Vulkan Semaphore");

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    err = vkCreateFence(device, &fenceInfo, nullptr, &inFlightFence);
    ALWAYS_ASSERT(!err, "Failed to create Vulkan Fence");

    err = vkCreateFence(device, &fenceInfo, nullptr, &immCommandFence_);
    ALWAYS_ASSERT(!err, "Failed to create Vulkan Fence");
}

void RenderDevice::createDepthResources()
{
    depthImage.init({
        hk::Image::Usage::DEPTH_STENCIL_ATTACHMENT,
        VK_FORMAT_D32_SFLOAT,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        VK_IMAGE_ASPECT_DEPTH_BIT,
        scExtent.width, scExtent.height, 1,
    });
}

VkShaderModule RenderDevice::createShaderModule(const hk::vector<u32> &code)
{
    VkResult err;

    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size() * sizeof(u32);
    createInfo.pCode = code.data();

    VkShaderModule shaderModule;
    err = vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule);
    ALWAYS_ASSERT(!err, "Failed to create Vulkan Shader Module");

    return shaderModule;
}

void RenderDevice::submitImmCmd(
    const std::function<void(VkCommandBuffer cmd)> &&callback) const
{
    VkResult err;

    err = vkResetFences(device, 1, &immCommandFence_);
    ALWAYS_ASSERT(!err, "Failed to reset Vulkan Fence");

    err = vkResetCommandBuffer(immCommandBuffer_, 0);
    ALWAYS_ASSERT(!err, "Failed to reset Vulkan immediate Command Buffer");

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    err = vkBeginCommandBuffer(immCommandBuffer_, &beginInfo);
    ALWAYS_ASSERT(!err, "Failed to begin immediate Command Buffer");

    callback(immCommandBuffer_);

    err = vkEndCommandBuffer(immCommandBuffer_);
    ALWAYS_ASSERT(!err, "Failed to end immediate Command Buffer");

    // VkCommandBufferSubmitInfo cmdInfo = {};
    // cmdInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    // cmdInfo.commandBuffer = immCommandBuffer_;
    // cmdInfo.deviceMask = 0;

    // VkSubmitInfo2 submitInfo = {};
    // submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    // submitInfo.commandBufferInfoCount = 1;
    // submitInfo.pCommandBufferInfos = &cmdInfo;

    // err = vkQueueSubmit2(graphicsQueue, 1, &submitInfo, immCommandFence_);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &immCommandBuffer_;

    err = vkQueueSubmit(graphicsQueue, 1, &submitInfo, immCommandFence_);
    ALWAYS_ASSERT(!err, "Failed to submit Vulkan immediate Command Buffer");

    err = vkWaitForFences(device, 1, &immCommandFence_, VK_TRUE, UINT64_MAX);
    ALWAYS_ASSERT(!err);
}

RenderDevice::DeviceBuffer
RenderDevice::createBuffer(const BufferDesc &desc) const
{
    VkResult err;

    DeviceBuffer out;

    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = desc.size;
    bufferInfo.usage = desc.usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    err = vkCreateBuffer(device, &bufferInfo, nullptr, &out.buffer);
    ALWAYS_ASSERT(!err, "Failed to create Vulkan Buffer");

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, out.buffer, &memRequirements);

    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    u32 memIndex = 0;
    for (; memIndex < memProperties.memoryTypeCount; ++memIndex) {
        if ((memRequirements.memoryTypeBits & (1 << memIndex)) &&
            (memProperties.memoryTypes[memIndex].propertyFlags &
            desc.properties) == desc.properties)
        {
            break;
        }
    }

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = memIndex;

    err = vkAllocateMemory(device, &allocInfo, nullptr, &out.bufferMemory);
    ALWAYS_ASSERT(!err, "Failed to allocate Vulkan Buffer Memory");

    vkBindBufferMemory(device, out.buffer, out.bufferMemory, 0);

    return out;
}

void RenderDevice::copyBuffer(VkBuffer srcBuffer,
                              VkBuffer dstBuffer,
                              VkDeviceSize size) const
{
    submitImmCmd([&](VkCommandBuffer cmd) {
        VkBufferCopy copyRegion = {};
        copyRegion.size = size;
        vkCmdCopyBuffer(cmd, srcBuffer, dstBuffer, 1, &copyRegion);
    });
}

RenderDevice::DeviceImage
RenderDevice::createImage(const ImageDesc &desc) const
{
    VkResult err;

    DeviceImage out;

    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = desc.width;
    imageInfo.extent.height = desc.height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = desc.format;
    imageInfo.tiling = desc.tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = desc.usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    err = vkCreateImage(device, &imageInfo, nullptr, &out.image);

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, out.image, &memRequirements);

    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    u32 memIndex = 0;
    for (; memIndex < memProperties.memoryTypeCount; ++memIndex) {
        if ((memRequirements.memoryTypeBits & (1 << memIndex)) &&
            (memProperties.memoryTypes[memIndex].propertyFlags &
            desc.properties) == desc.properties)
        {
            break;
        }
    }

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = memIndex;

    err = vkAllocateMemory(device, &allocInfo, nullptr, &out.imageMemory);

    vkBindImageMemory(device, out.image, out.imageMemory, 0);

    return out;
}

void RenderDevice::transitionImageLayout(
    VkImage image,
    VkFormat format,
    VkImageLayout oldLayout,
    VkImageLayout newLayout)
{
    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
        newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
               newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
               newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask =
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    } else {
        LOG_ERROR("Unsupported layout transition");
    }

    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

        if (format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
            format == VK_FORMAT_D24_UNORM_S8_UINT)
        {
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    } else {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    submitImmCmd([&](VkCommandBuffer cmd) {
        vkCmdPipelineBarrier(
            cmd,
            sourceStage, destinationStage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );
    });
}

void RenderDevice::copyBufferToImage(
    VkBuffer buffer, VkImage image,
    u32 width, u32 height)
{
    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = { width, height, 1 };

    submitImmCmd([&](VkCommandBuffer cmd) {

        vkCmdCopyBufferToImage(
            cmd,
            buffer,
            image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &region);
    });
}

static VKAPI_ATTR VkBool32 VKAPI_CALL
vkDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
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
