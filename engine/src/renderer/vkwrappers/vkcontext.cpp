#include "vkcontext.h"

namespace hk {

static struct Context {
    VkInstance instance_ = VK_NULL_HANDLE;
    InstanceInfo instanceInfo_;

    i32 physicalDeviceIndex_ = -1;
    hk::vector<PhysicalDeviceInfo> physicalDevicesInfo_;

    LogicalDeviceInfo deviceInfo_;
    VkDevice device_ = VK_NULL_HANDLE;

    Queue graphics_;
    Queue compute_;
    Queue transfer_;
} ctx;

// class ResourceManager {
//
//     hk::vector<hk::Image> images_;
//     hk::vector<hk::Buffer> buffers_;
//
//     hk::vector<hk::Pipeline> pipelines_;
//     // Descriptors?
//
//     hk::vector<hk::RenderMaterial> materials_;
//
//     // Shader Modules?
// };

}
