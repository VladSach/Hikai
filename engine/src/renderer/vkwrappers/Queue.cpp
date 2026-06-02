#include "Queue.h"

#include "hkstl/numerics/hkbit.h"
#include "hkstl/containers/hkvector.h"

namespace hk {

void QueueFamily::findQueue(
    const VkPhysicalDevice physical,
    VkQueueFlags flags,
    const VkSurfaceKHR surface)
{
    u32 q_family_cnt = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical,
                                             &q_family_cnt,
                                             nullptr);

    hk::vector<VkQueueFamilyProperties> queueFamilies(q_family_cnt);
    vkGetPhysicalDeviceQueueFamilyProperties(physical,
                                             &q_family_cnt,
                                             queueFamilies.data());

    // best match index
    u32 target_idx = VK_QUEUE_FAMILY_IGNORED;

    u32 extraQueuesMin = hk::popcount(
        static_cast<VkQueueFlags>(VK_QUEUE_FLAG_BITS_MAX_ENUM));

    for (u32 i = 0; i < q_family_cnt; ++i) {
        const VkQueueFamilyProperties &q_family = queueFamilies[i];
        const VkQueueFlags &q_flags = q_family.queueFlags;

        // Skip if doesn't contain the required flags
        if (!(q_flags & flags)) { continue; }

        // Skip if a surface is provided and that surface
        // isn't supported by the queue
        if (surface) {
            VkBool32 presentSupport;
            vkGetPhysicalDeviceSurfaceSupportKHR(physical, i,
                                                 surface,
                                                 &presentSupport);
            if (!presentSupport) {
                continue;
            }
        }

        u32 extraQueues = hk::popcount(q_flags & ~flags);


        // TODO: improve queue search
        // first matching queue?  use it and continue
        // if (targetIndex == UINT32_MAX) {
        //     targetIndex = i;
        //     targetFlags = queueFamilies[i].queueFlags;
        //     continue;
        // }
        if (extraQueues == 0) {
            index_ = i;
            break;
        }

        // Matching queue, but with fewer flags than the current best?  Use it.
        // if (countBits(queueFamilies[i].queueFlags) <
        //     countBits(targetFlags)) {
        //     targetIndex = i;
        //     targetFlags = queueFamilies[i].queueFlags;
        //     continue;
        // }
        if (target_idx == VK_QUEUE_FAMILY_IGNORED ||
            extraQueues < extraQueuesMin)
        {
            target_idx = i;
            extraQueuesMin = extraQueues;
        }

    }


    index_ = target_idx;
    properties = queueFamilies[target_idx];
}

void Queue::init(VkDevice device, QueueFamily family)
{
    device_ = device;
    family_ = family;

    vkGetDeviceQueue(device_, family_.index_, 0, &handle_);
    createCommandPool();
}

void Queue::deinit()
{
    if (pool_) {
        vkDestroyCommandPool(device_, pool_, nullptr);
        pool_ = VK_NULL_HANDLE;
    }
}

void Queue::createCommandPool(VkCommandPoolCreateFlags flags)
{
    VkResult err;

    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = flags;
    poolInfo.queueFamilyIndex = family_.index_;

    err = vkCreateCommandPool(device_, &poolInfo, nullptr, &pool_);
    ALWAYS_ASSERT(!err, "Failed to create Vulkan Command Pool");
}

VkCommandBuffer Queue::createCommandBuffer(VkCommandBufferLevel level) const
{
    VkResult err;

    VkCommandBuffer out;

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = level;
    allocInfo.commandPool = pool_;
    allocInfo.commandBufferCount = 1;

    err = vkAllocateCommandBuffers(device_, &allocInfo, &out);
    ALWAYS_ASSERT(!err, "Failed to allocate Vulkan Command Buffer");

    return out;
}

void Queue::freeCommandBuffer(VkCommandBuffer cmd) const
{
    vkFreeCommandBuffers(device_, pool_, 1, &cmd);
}

VkResult Queue::submit(
    VkCommandBuffer cmd, VkFence fence,
    VkSemaphore signal, VkSemaphore wait) const
{
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;
    if (signal) {
        submitInfo.pSignalSemaphores = &signal;
        submitInfo.signalSemaphoreCount = 1;
    }
    if (wait) {
        submitInfo.pWaitSemaphores = &wait;
        submitInfo.waitSemaphoreCount = 1;
    }
    return vkQueueSubmit(handle_, 1, &submitInfo, fence);
}

void Queue::submitAndWait(VkCommandBuffer cmd) const
{
    VkResult err;

    VkFence fence;
    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    err = vkCreateFence(device_, &fenceInfo, nullptr, &fence);
    ALWAYS_ASSERT(!err, "Failed to create Vulkan Fence");

    vkResetFences(device_, 1, &fence);

    err = submit(cmd, fence);
    ALWAYS_ASSERT(!err, "Failed to submit Vulkan Command Buffer");

    err = vkWaitForFences(device_, 1, &fence, VK_TRUE, UINT64_MAX);
    ALWAYS_ASSERT(err != VK_TIMEOUT, "Vulkan Fence timeouted");

    vkDestroyFence(device_, fence, VK_NULL_HANDLE);
}

VkResult Queue::present(VkSwapchainKHR sc, u32 index, VkSemaphore semaphore)
{
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &sc;
    presentInfo.pImageIndices = &index;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &semaphore;

    return vkQueuePresentKHR(handle_, &presentInfo);
}

}
