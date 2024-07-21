#ifndef HK_UB_MANAGER_H
#define HK_UB_MANAGER_H

#include "math/hkmath.h"

// FIX: temp
#include "vendor/vulkan/vulkan.h"
VkDescriptorSet *getGlobalDescriptorSet();
VkDescriptorSetLayout getGlobalDescriptorSetLayout();
#include "Descriptors.h"
hk::DescriptorWriter *getGlobalDescriptorWriter();

namespace hk::ubo {

// TODO: reinforce alignment to 16 bits
struct SceneData {
    hkm::vec2f resolution;
    f32 time;
};

void init();
void deinit();

void setFrameData(const SceneData &ubo);

}

#endif // HK_UB_MANAGER_H
