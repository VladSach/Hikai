#ifndef HK_DESCRIPTOR_SET_LAYOUT_H
#define HK_DESCRIPTOR_SET_LAYOUT_H

#include "vendor/vulkan/vulkan.h"
#include "utils/containers/hkvector.h"
#include "renderer/RenderDevice.h"

// TODO: replace
#include <deque>
#include <unordered_map>

namespace hk {

class DescriptorLayout {
public:
    class Builder {
    public:

        Builder& addBinding(u32 binding,
                            VkDescriptorType type,
                            VkShaderStageFlags stages);

        DescriptorLayout build();

    private:
        // hk::vector<VkDescriptorSetLayoutBinding> bindings_;
        std::unordered_map<u32, VkDescriptorSetLayoutBinding> bindings_ = {};
    };

    DescriptorLayout(hk::vector<VkDescriptorSetLayoutBinding> bindings)
    {
        init(bindings);
    }
    ~DescriptorLayout() { deinit(); }

    void init(hk::vector<VkDescriptorSetLayoutBinding> bindings);
    void deinit();

    constexpr VkDescriptorSetLayout layout() const { return layout_; }

private:
    VkDescriptorSetLayout layout_ = VK_NULL_HANDLE;

    VkDevice device_ = hk::device()->logical();
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

    VkDevice device_ = hk::device()->logical();
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

    VkDevice device_ = hk::device()->logical();
};

}

// FIX: Find a better way to manage global objects and place them together
namespace hk {
inline DescriptorAllocator *pool()
{
    static DescriptorAllocator *globalDescriptorPool = nullptr;

    if (globalDescriptorPool == nullptr) {
        globalDescriptorPool = new DescriptorAllocator();

        hk::vector<hk::DescriptorAllocator::TypeSize> sizes =
        {
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10 }
        };
        globalDescriptorPool->init(10, sizes);
    }
    return globalDescriptorPool;
}
}

#endif // HK_DESCRIPTOR_SET_LAYOUT_H