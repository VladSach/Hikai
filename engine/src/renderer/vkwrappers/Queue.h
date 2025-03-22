#ifndef HK_QUEUE_H
#define HK_QUEUE_H

#include "vendor/vulkan/vulkan.h"

#include "hkcommon.h"
#include "hkstl/containers/hkvector.h"

namespace hk {

struct QueueFamily {
    u32 index_ = VK_QUEUE_FAMILY_IGNORED;
    VkQueueFamilyProperties properties;

    constexpr operator bool() const
    {
        return index_ != VK_QUEUE_FAMILY_IGNORED;
    }

    void findQueue(const VkPhysicalDevice physical,
                   VkQueueFlags flags,
                   const VkSurfaceKHR surface = nullptr);
};

class Queue {
public:
    Queue() = default;
    ~Queue() { deinit(); }

    void init(VkDevice device, QueueFamily family);

    void deinit();

    void createCommandPool(VkCommandPoolCreateFlags flags =
                           VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    VkCommandBuffer createCommandBuffer(VkCommandBufferLevel level =
                                        VK_COMMAND_BUFFER_LEVEL_PRIMARY) const;

    void freeCommandBuffer(VkCommandBuffer cmd) const;

    VkResult submit(
        VkCommandBuffer cmd,
        VkFence fence,
        VkSemaphore signal = VK_NULL_HANDLE,
        VkSemaphore wait = VK_NULL_HANDLE) const;

    void submitAndWait(VkCommandBuffer cmd) const;

    VkResult present(VkSwapchainKHR sc, u32 index, VkSemaphore semaphore);

public:
    constexpr u32 family() const { return family_.index_; }
    constexpr VkQueue handle() const { return handle_; }

private:
    QueueFamily family_;
    VkQueue handle_ = VK_NULL_HANDLE;
    VkCommandPool pool_ = VK_NULL_HANDLE;

    VkDevice device_ = VK_NULL_HANDLE;
};

}

#endif // HK_QUEUE_H
