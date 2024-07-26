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
/*
    * Scalars have to be aligned by N (= 4 bytes given 32 bit floats).
    * A vec2 must be aligned by 2N (= 8 bytes)
    * A vec3 or vec4 must be aligned by 4N (= 16 bytes)
    * A nested structure must be aligned by the base alignment of its members
      rounded up to a multiple of 16.
    * A mat4 matrix must have the same alignment as a vec4.
*/
struct SceneData {
    hkm::vec2f resolution;
    f32 time;
    alignas(16)hkm::mat4f viewProjection;
};

void init();
void deinit();

void setFrameData(const SceneData &ubo);

}

#endif // HK_UB_MANAGER_H
