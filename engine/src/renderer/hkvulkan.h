#ifndef HK_VULKAN_H
#define HK_VULKAN_H

#include "vendor/vulkan/vulkan.h"

// FIX: move context to vkwrappers and rename context or device context
#include "renderer/VulkanContext.h"
#include "renderer/vkwrappers/Buffer.h"
#include "renderer/vkwrappers/Descriptors.h"
#include "renderer/vkwrappers/Image.h"
#include "renderer/vkwrappers/Pipeline.h"
#include "renderer/vkwrappers/Queue.h"
#include "renderer/vkwrappers/Swapchain.h"
#include "renderer/vkwrappers/vkdebug.h"
#include "renderer/vkwrappers/vkutils.h"

#endif // HK_VULKAN_H
