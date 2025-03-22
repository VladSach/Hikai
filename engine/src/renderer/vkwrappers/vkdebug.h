#ifndef HK_VKDEBUG_H
#define HK_VKDEBUG_H

#include "vendor/vulkan/vulkan.h"

#include "math/vec4f.h"

#include "hkstl/containers/hkvector.h"
#include "hkstl/utility/hkassert.h"

#define CHECK_DEVICE_LIMIT(op, value, limit)              \
    if (!((value) op (limit))) {                          \
        DEV_PANIC("Hikai Vulkan Limit Failure",           \
                  "Exceeded `", #limit, "` value\n",      \
                  "allowed:", limit, "provided:", value); \
    }

namespace hk::debug {

/* ===== Validation ===== */
void init(VkInstance instance);
void deinit(VkInstance instance);
VkDebugUtilsMessengerCreateInfoEXT& info();

namespace label {

void begin(VkCommandBuffer cmd, const std::string &name, const hkm::vec4f &color);
void end(VkCommandBuffer cmd);

void insert(VkCommandBuffer cmd, const std::string &name, const hkm::vec4f &color);

} // namespace label

/* ===== Object naming and tagging ===== */
void setDevice(VkDevice dev);

void setName(VkInstance            instance,    const std::string &name);
void setName(VkPhysicalDevice      device,      const std::string &name);
void setName(VkDevice              device,      const std::string &name);
void setName(VkQueue               queue,       const std::string &name);
void setName(VkSemaphore           semaphore,   const std::string &name);
void setName(VkCommandBuffer       cmd,         const std::string &name);
void setName(VkFence               fence,       const std::string &name);
void setName(VkDeviceMemory        memory,      const std::string &name);
void setName(VkBuffer              buf,         const std::string &name);
void setName(VkImage               image,       const std::string &name);
void setName(VkEvent               event,       const std::string &name);
void setName(VkQueryPool           pool,        const std::string &name);
void setName(VkBufferView          view,        const std::string &name);
void setName(VkImageView           view,        const std::string &name);
void setName(VkShaderModule        shader,      const std::string &name);
void setName(VkPipelineCache       cache,       const std::string &name);
void setName(VkPipelineLayout      layout,      const std::string &name);
void setName(VkRenderPass          pass,        const std::string &name);
void setName(VkPipeline            pipeline,    const std::string &name);
void setName(VkDescriptorSetLayout layout,      const std::string &name);
void setName(VkSampler             sampler,     const std::string &name);
void setName(VkDescriptorPool      pool,        const std::string &name);
void setName(VkDescriptorSet       set,         const std::string &name);
void setName(VkFramebuffer         framebuffer, const std::string &name);
void setName(VkCommandPool         pool,        const std::string &name);

} // namespace hk::debug

#endif // HK_VKDEBUG_H
