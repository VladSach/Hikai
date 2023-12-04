#include "BackendVulkan.h"

#include "vendor/vulkan/vulkan.h"
#include "vendor/vulkan/vulkan_win32.h"

// TODO: change with hikai implementation
#include <set>

static VKAPI_ATTR VkBool32 VKAPI_CALL vkDebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData);

void BackendVulkan::init(const Window &window)
{
    LOG_INFO("Initializing Vulkan Backend");

    createInstance();
    createSurface(window);
    createPhysicalDevice();
    createLogicalDevice();
    createSwapchain();
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createFramebuffers();
    createCommandPool();
    createCommandBuffer();
    createSyncObjects();
}

void BackendVulkan::cleanupSwapchain()
{
    for (auto framebuffer : scFramebuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }

    for (auto imageView : scImageViews) {
        vkDestroyImageView(device, imageView, nullptr);
    }

    vkDestroySwapchainKHR(device, swapchain, nullptr);
}

void BackendVulkan::deinit()
{
    cleanupSwapchain();

    vkDestroyPipeline(device, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vkDestroyRenderPass(device, renderPass, nullptr);

    vkDestroySemaphore(device, aquireSemaphore, nullptr);
    vkDestroySemaphore(device, submitSemaphore, nullptr);
    vkDestroyFence(device, inFlightFence, nullptr);

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

void BackendVulkan::draw()
{
    // TODO: rewrite this whole thing
    vkWaitForFences(device, 1, &inFlightFence, VK_TRUE, UINT64_MAX);

    u32 imageIndex;
    vkAcquireNextImageKHR(device, swapchain, 0,
                          aquireSemaphore, 0, &imageIndex);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkClearValue clearValue = {};
    clearValue.color = {214.f/256.f, 2.f/256.f, 112.f/256.f, 1.f};

    VkRenderPassBeginInfo rpBeginInfo = {};
    rpBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpBeginInfo.renderPass = renderPass;
    rpBeginInfo.renderArea.extent = VkExtent2D{1020, 780};
    rpBeginInfo.framebuffer = scFramebuffers[imageIndex];
    rpBeginInfo.pClearValues = &clearValue;
    rpBeginInfo.clearValueCount = 1;

    vkCmdBeginRenderPass(commandBuffer, &rpBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    {
        // render
    }
    vkCmdEndRenderPass(commandBuffer);
    vkEndCommandBuffer(commandBuffer);

    vkResetFences(device, 1, &inFlightFence);

    VkPipelineStageFlags waitStage =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    submitInfo.pSignalSemaphores = &submitSemaphore;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &aquireSemaphore;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitDstStageMask = &waitStage;
    vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFence);

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pWaitSemaphores = &submitSemaphore;
    presentInfo.waitSemaphoreCount = 1;
    vkQueuePresentKHR(presentQueue, &presentInfo);
}

void BackendVulkan::createInstance()
{
    VkResult err;

    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Blight"; // TODO: make configurable
    appInfo.pEngineName = "Hikai Engine";

	u32 extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

	hk::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount,
	                                       availableExtensions.data());

	hk::vector<const char*> extensions;

	for (auto& availableExtension : availableExtensions) {
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

#ifdef _WIN32
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
    instanceInfo.enabledLayerCount = sizeof(layers)/sizeof(layers[0]);

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

void BackendVulkan::createPhysicalDevice()
{
    u32 deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    ALWAYS_ASSERT(deviceCount, "Failed to find GPUs with Vulkan support");

    hk::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    LOG_DEBUG("List of all found GPUs:");

    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    VkPhysicalDeviceMemoryProperties deviceMemoryProperties;
    for (const auto &deviceP : devices) {
        vkGetPhysicalDeviceProperties(deviceP, &deviceProperties);
        vkGetPhysicalDeviceFeatures(deviceP, &deviceFeatures);
        vkGetPhysicalDeviceMemoryProperties(deviceP, &deviceMemoryProperties);

        LOG_TRACE(deviceProperties.deviceName);

        if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            physicalDevice = deviceP;
            break;
        }
        LOG_WARN("Non discrete GPUs are not supported by Hikari");
    }

    LOG_DEBUG("Chosen GPU:", deviceProperties.deviceName);

    ALWAYS_ASSERT(physicalDevice != VK_NULL_HANDLE,
                  "Failed to find a suitable GPU");
}

void BackendVulkan::createLogicalDevice()
{
    VkResult err;

    // Pick queue families
    u32 queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice,
	                                         &queueFamilyCount, nullptr);

    hk::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount,
                                             queueFamilies.data());

    for (u32 i = 0; i < queueFamilyCount; i++) {
        auto& queueFamily = queueFamilies[i];

		if (graphicsFamily == VK_QUEUE_FAMILY_IGNORED &&
		    queueFamily.queueCount > 0 &&
		    queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			graphicsFamily = i;
		}

		if (transferFamily == VK_QUEUE_FAMILY_IGNORED &&
		    queueFamily.queueCount > 0 &&
		    queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT)
		{
			transferFamily = i;
		}

		if (computeFamily == VK_QUEUE_FAMILY_IGNORED &&
		    queueFamily.queueCount > 0 &&
		    queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)
		{
			computeFamily = i;
		}
	}

    // Create queue families
    f32 queuePriority = 1.f;
    hk::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<u32> uniqueQueueFamilies = { graphicsFamily, computeFamily,
                                          transferFamily };
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

    VkDeviceCreateInfo deviceInfo = {};
    deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceInfo.pQueueCreateInfos = queueCreateInfos.data();
    deviceInfo.queueCreateInfoCount = static_cast<u32>(queueCreateInfos.size());
    deviceInfo.pEnabledFeatures = nullptr;
    deviceInfo.ppEnabledExtensionNames = extensionsLogic;
    deviceInfo.enabledExtensionCount = sizeof(extensionsLogic) /
                                       sizeof(extensionsLogic[0]);

    err = vkCreateDevice(physicalDevice, &deviceInfo, 0, &device);
    ALWAYS_ASSERT(!err, "Failed to create Vulkan Logical Device");

    // Get queue handles
    vkGetDeviceQueue(device, graphicsFamily, 0, &graphicsQueue);
    vkGetDeviceQueue(device, computeFamily,  0, &computeQueue);
    vkGetDeviceQueue(device, transferFamily, 0, &transferQueue);

    VkBool32 presentSupport;
    for (u32 i = 0; i < queueFamilyCount; i++) {
		presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i,
		                                     surface, &presentSupport);
		if (presentFamily == VK_QUEUE_FAMILY_IGNORED &&
		    queueFamilies[i].queueCount > 0 &&
		    presentSupport)
		{
			presentFamily = i;
			break;
		}
	}

	ALWAYS_ASSERT(presentFamily != VK_QUEUE_FAMILY_IGNORED,
	              "Failed to find present queue");
    vkGetDeviceQueue(device, presentFamily,  0, &presentQueue);
}

void BackendVulkan::createSurface(const Window &window)
{
    VkResult err;

    // Create window surface
#ifdef _WIN32
    VkWin32SurfaceCreateInfoKHR surfaceInfo = {};
    surfaceInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surfaceInfo.hwnd = window.getHWnd();
    surfaceInfo.hinstance = window.getHInstance();
#endif

    err = vkCreateWin32SurfaceKHR(instance, &surfaceInfo, 0, &surface);
    ALWAYS_ASSERT(!err, "Failed to create Vulkan Surface");
}

void BackendVulkan::createSwapchain()
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
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface,
                                              &presentModeCount,
                                              presentModes.data());

    for (const auto &mode : presentModes) {
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            presentMode = mode;
            break;
        }

        // FIFO mode is the one that is always supported
        presentMode = VK_PRESENT_MODE_FIFO_KHR;
    }

    u32 imageCount = surfaceCaps.minImageCount + 1 < surfaceCaps.maxImageCount
                        ? surfaceCaps.minImageCount + 1
                        : surfaceCaps.maxImageCount;

    VkSwapchainCreateInfoKHR swapchainInfo = {};
    swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                               VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainInfo.surface = surface;
    swapchainInfo.imageFormat = surfaceFormat.format;
    swapchainInfo.imageColorSpace = surfaceFormat.colorSpace;
    swapchainInfo.preTransform = surfaceCaps.currentTransform;
    swapchainInfo.imageExtent = surfaceCaps.currentExtent;
    swapchainInfo.minImageCount = imageCount;
    swapchainInfo.imageArrayLayers = 1;
    swapchainInfo.presentMode = presentMode;
    swapchainInfo.clipped = VK_TRUE;

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

void BackendVulkan::createImageViews()
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

void BackendVulkan::createRenderPass()
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

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    err = vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass);
    ALWAYS_ASSERT(!err, "Failed to create Vulkan Render Pass");
}

void BackendVulkan::createGraphicsPipeline()
{
    // VkResult err;
    // auto vertShaderCode = readFile("shaders/vert.spv");
    // auto fragShaderCode = readFile("shaders/frag.spv");

    // VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
    // vertShaderStageInfo.sType =
    //     VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    // vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    // vertShaderStageInfo.module = vertShaderModule;
    // vertShaderStageInfo.pName = "main";

    // VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
    // fragShaderStageInfo.sType =
    //     VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    // fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    // fragShaderStageInfo.module = fragShaderModule;
    // fragShaderStageInfo.pName = "main";
    //
    // VkPipelineShaderStageCreateInfo shaderStages[] =
    //     { vertShaderStageInfo, fragShaderStageInfo };
    //
    // vkDestroyShaderModule(device, fragShaderModule, nullptr);
    // vkDestroyShaderModule(device, vertShaderModule, nullptr);


    // VkGraphicsPipelineCreateInfo pipelineInfo = {};
    // pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    // pipelineInfo.stageCount = 2;
    // // pipelineInfo.pStages = shaderStages;
    // pipelineInfo.pVertexInputState = &vertexInputInfo;
    // pipelineInfo.pInputAssemblyState = &inputAssembly;
    // pipelineInfo.pViewportState = &viewportState;
    // pipelineInfo.pRasterizationState = &rasterizer;
    // pipelineInfo.pMultisampleState = &multisampling;
    // pipelineInfo.pDepthStencilState = nullptr;
    // pipelineInfo.pColorBlendState = &colorBlending;
    // pipelineInfo.pDynamicState = &dynamicState;
    // pipelineInfo.layout = pipelineLayout;
    //
    // pipelineInfo.renderPass = renderPass;
    // pipelineInfo.subpass = 0;
    //
    // err = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo,
    //                                 nullptr, &graphicsPipeline);
    // ALWAYS_ASSERT(!err, "Failed to create Vulkan Graphics Pipeline");
}

void BackendVulkan::createFramebuffers()
{
    VkResult err;

    scFramebuffers.resize(scImageViews.size());
    for (u32 i = 0; i < scImageViews.size(); i++) {
        VkImageView attachments[] = {
            scImageViews[i]
        };

        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        // framebufferInfo.width = scExtent.width;
        // framebufferInfo.height = scExtent.height;
        // FIX: change to correct version
        framebufferInfo.width = 1020;
        framebufferInfo.height = 780;
        framebufferInfo.layers = 1;

        err = vkCreateFramebuffer(device, &framebufferInfo,
                                  nullptr, &scFramebuffers[i]);
        ALWAYS_ASSERT(!err, "Failed to create Vulkan Frambuffer");
    }
}

void BackendVulkan::createCommandPool()
{
    VkResult err;

    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = graphicsFamily;

    err = vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool);
    ALWAYS_ASSERT(!err, "Failed to create Vulkan Command Pool");
}

void BackendVulkan::createCommandBuffer()
{
    VkResult err;

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    err = vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);
    ALWAYS_ASSERT(!err, "Failed to allocate Vulkan Command Buffer");
}

void BackendVulkan::createSyncObjects()
{
    VkResult err;

    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    err = vkCreateSemaphore(device, &semaphoreInfo, nullptr, &aquireSemaphore);
    ALWAYS_ASSERT(!err, "Failed to create Vulkan Semaphore");

    err = vkCreateSemaphore(device, &semaphoreInfo, nullptr, &submitSemaphore);
    ALWAYS_ASSERT(!err, "Failed to create Vulkan Semaphore");

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    err = vkCreateFence(device, &fenceInfo, nullptr, &inFlightFence);
    ALWAYS_ASSERT(!err, "Failed to create Vulkan Fence");
}

VkShaderModule BackendVulkan::createShaderModule(const std::vector<char>& code)
{
    VkResult err;

    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    err = vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule);
    ALWAYS_ASSERT(!err, "Failed to create Vulkan Shader Module");

    return shaderModule;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL vkDebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
    (void)messageType; (void)pUserData;

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

    return VK_FALSE;
}
