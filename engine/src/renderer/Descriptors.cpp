#include "Descriptors.h"

namespace hk {

DescriptorLayout::Builder& DescriptorLayout::Builder::addBinding(
    u32 binding,
    VkDescriptorType type,
    VkShaderStageFlags stages)
{
    ALWAYS_ASSERT(!bindings_.count(binding),
                  "Descriptor binding:", binding, "is already in use");

    VkDescriptorSetLayoutBinding layout = {};
    layout.binding = binding;
    layout.descriptorCount = 1;
    layout.descriptorType = type;
    layout.stageFlags = stages;
    // layout.pImmutableSamplers = VK_NULL_HANDLE;

    bindings_[binding] = layout;

    return *this;
}

DescriptorLayout DescriptorLayout::Builder::build()
{
    hk::vector<VkDescriptorSetLayoutBinding> bindings;
    for (auto &binding : bindings_) {
        bindings.push_back(binding.second);
    }

    return DescriptorLayout(bindings);
}

void DescriptorLayout::init(hk::vector<VkDescriptorSetLayoutBinding> bindings)
{
    VkResult err;

    VkDescriptorSetLayoutCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    info.pBindings = bindings.data();
    info.bindingCount = bindings.size();
    // info.pNext = nullptr;
    // info.flags = 0;

    err = vkCreateDescriptorSetLayout(device_, &info, nullptr, &layout_);
    ALWAYS_ASSERT(!err, "Failed to create Vulkan Descriptor Set Layout");
}

void DescriptorLayout::deinit()
{
    vkDestroyDescriptorSetLayout(device_, layout_, nullptr);
    layout_ = VK_NULL_HANDLE;
}

void DescriptorAllocator::init(
    u32 maxSets,
    const hk::vector<TypeSize> &sizes)
{
    sizes_.clear();

    for (auto size : sizes) {
        sizes_.push_back(size);
    }

    VkDescriptorPool newPool = createPool(maxSets, sizes_);

    // Grow it next allocation
    setsPerPool = static_cast<u32>(maxSets * 1.5f);

    readyPools.push_back(newPool);
}

void DescriptorAllocator::deinit()
{
    for (auto pool : readyPools) {
        vkDestroyDescriptorPool(device_, pool, nullptr);
    }
    readyPools.clear();

    for (auto pool : fullPools) {
        vkDestroyDescriptorPool(device_, pool, nullptr);
    }
    fullPools.clear();
}

void DescriptorAllocator::clear()
{
    for (auto pool : readyPools) {
        vkResetDescriptorPool(device_, pool, 0);
    }

    for (auto pool : fullPools) {
        vkResetDescriptorPool(device_, pool, 0);
        readyPools.push_back(pool);
    }

    fullPools.clear();
}

VkDescriptorSet DescriptorAllocator::allocate(
    VkDescriptorSetLayout layout,
    void *pNext)
{
    VkResult err;

    // Get or create a pool to allocate from
    VkDescriptorPool poolToUse = getPool();

    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.pNext = pNext;
    allocInfo.descriptorPool = poolToUse;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &layout;

    VkDescriptorSet descriptorSet;
    err = vkAllocateDescriptorSets(device_, &allocInfo, &descriptorSet);

    if (err == VK_ERROR_OUT_OF_POOL_MEMORY || err == VK_ERROR_FRAGMENTED_POOL) {
        fullPools.push_back(poolToUse);

        poolToUse = getPool();
        allocInfo.descriptorPool = poolToUse;

        err = vkAllocateDescriptorSets(device_, &allocInfo, &descriptorSet);
        ALWAYS_ASSERT(!err, "Failed to allocate Vulkan Descriptor Set");
    }

    readyPools.push_back(poolToUse);
    return descriptorSet;
}

VkDescriptorPool DescriptorAllocator::getPool()
{
    VkDescriptorPool pool;

    if (readyPools.size() != 0) {
        pool = readyPools.back();
        readyPools.pop_back();
    } else {
        // Need to create a new pool
        pool = createPool(setsPerPool, sizes_);

        setsPerPool = static_cast<u32>(setsPerPool * 1.5f);
        if (setsPerPool > 4092) {
            setsPerPool = 4092;
        }
    }

    return pool;
}

VkDescriptorPool DescriptorAllocator::createPool(
    u32 maxSets,
    const hk::vector<TypeSize> &sizes)
{
    hk::vector<VkDescriptorPoolSize> poolSizes;
    for (const TypeSize &size : sizes) {
        poolSizes.push_back({ size.type, size.size });
    }

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = 0;
    poolInfo.maxSets = maxSets;
    poolInfo.poolSizeCount = poolSizes.size();
    poolInfo.pPoolSizes = poolSizes.data();

    VkDescriptorPool pool;
    vkCreateDescriptorPool(device_, &poolInfo, nullptr, &pool);

    return pool;
}

void DescriptorWriter::writeBuffer(
    u32 binding,
    VkBuffer buffer,
    u32 size,
    u32 offset,
    VkDescriptorType type)
{
    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer = buffer;
    bufferInfo.offset = offset;
    bufferInfo.range = size;

    VkDescriptorBufferInfo &info = bufferInfos.emplace_back(bufferInfo);

    VkWriteDescriptorSet write = {};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstBinding = binding;
    write.descriptorCount = 1;
    write.descriptorType = type;
    write.pBufferInfo = &info;

    write.dstSet = VK_NULL_HANDLE; // left empty until we need to write it

    writes.push_back(write);
}

void DescriptorWriter::writeImage(
    u32 binding,
    VkImageView image,
    VkSampler sampler,
    VkImageLayout layout,
    VkDescriptorType type)
{
    VkDescriptorImageInfo imageInfo = {};
    imageInfo.imageLayout = layout;
    imageInfo.imageView = image;
    imageInfo.sampler = sampler;

    VkDescriptorImageInfo &info = imageInfos.emplace_back(imageInfo);

    VkWriteDescriptorSet write = {};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstBinding = binding;
    write.descriptorCount = 1;
    write.descriptorType = type;
    write.pImageInfo = &info;

    write.dstSet = VK_NULL_HANDLE; // left empty until we need to write it

    writes.push_back(write);
}

void DescriptorWriter::clear()
{
    imageInfos.clear();
    bufferInfos.clear();
    writes.clear();
}

void DescriptorWriter::updateSet(VkDescriptorSet set)
{
    for (VkWriteDescriptorSet &write : writes) {
        write.dstSet = set;
    }

    vkUpdateDescriptorSets(device_, writes.size(), writes.data(), 0, nullptr);
}

}
