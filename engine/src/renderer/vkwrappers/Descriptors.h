#ifndef HK_DESCRIPTOR_SET_LAYOUT_H
#define HK_DESCRIPTOR_SET_LAYOUT_H

#include "vendor/vulkan/vulkan.h"

#include "renderer/vkwrappers/vkcontext.h"

#include "hkstl/containers/hkvector.h"

// TODO: replace
#include <deque>
#include <unordered_map>

namespace hk {

class DescriptorLayout {
public:
    class Builder {
    public:
        Builder& addBinding(u32 binding, u32 count,
                            VkDescriptorType type,
                            VkShaderStageFlags stages,
                            VkSampler *immutable_samplers = VK_NULL_HANDLE);

        hk::vector<VkDescriptorSetLayoutBinding> build();

    private:
        std::unordered_map<u32, VkDescriptorSetLayoutBinding> bindings_ = {};
    };

public:
    DescriptorLayout() = default;
    ~DescriptorLayout() { deinit(); }

    // If bindings is empty - creates null/dummy descriptor layout
    void init(
        const hk::vector<VkDescriptorSetLayoutBinding> &bindings,
        VkDescriptorSetLayoutCreateFlags flags = 0);
    void deinit();

    constexpr VkDescriptorSetLayout handle() const
    {
        DEV_ASSERT(handle_, "Trying to access VK_NULL_HANDLE");
        return handle_;
    }

private:
    VkDescriptorSetLayout handle_ = VK_NULL_HANDLE;
};


class DescriptorAllocator {
public:
    struct TypeSize {
        VkDescriptorType type;
        u32 size;
    };

    void init(u32 maxSets, const hk::vector<TypeSize> &sizes);
    void deinit();

    void clear();

    VkDescriptorSet allocate(VkDescriptorSetLayout layout,
                             void *pNext = nullptr);

private:
    VkDescriptorPool getPool();
    VkDescriptorPool createPool(u32 maxSets,
                                const hk::vector<TypeSize> &sizes);

private:
    u32 setsPerPool;
    hk::vector<TypeSize> sizes_;
    hk::vector<VkDescriptorPool> fullPools;
    hk::vector<VkDescriptorPool> readyPools;
};

class DescriptorWriter {
public:
    void writeBuffer(u32 binding,
                     VkBuffer buffer,
                     u32 size,
                     u32 offset,
                     VkDescriptorType type);
    void writeImage(u32 binding,
                    VkImageView image,
                    VkSampler sampler,
                    VkImageLayout layout,
                    VkDescriptorType type);

    void clear();
    void updateSet(VkDescriptorSet set);

private:
    std::deque<VkDescriptorImageInfo> imageInfos;
    std::deque<VkDescriptorBufferInfo> bufferInfos;
    hk::vector<VkWriteDescriptorSet> writes;
};

}

#endif // HK_DESCRIPTOR_SET_LAYOUT_H
