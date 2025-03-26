#include "resources.h"

#include "hkvulkan.h"

#include "resource_pool.h"

#include <deque>

namespace hk::bkr {

struct InternalBuffer {
    VkBuffer handle = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
};

struct InternalImage {
    VkImage handle = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
};

struct VulkanImageDesc {
    u32 width, height;
    VkFormat format;
    VkImageTiling tiling;
    VkImageUsageFlags usage;
    VkImageAspectFlags aspect_mask;
    VkMemoryPropertyFlags properties;
};

static struct Resources {
    template <typename T>
    struct Slot {
        T data;
        u32 gen = 0;
        b8 is_valid = false;
    };

    // TODO: change to more suitable structure, maybe colony, maybe pool
    hk::vector<Slot<InternalBuffer>> buffer_pool;
    hk::vector<Slot<InternalImage>> image_pool;

    // TODO: change to hk::stack
    std::deque<u32> free_buffers;
    std::deque<u32> free_images;

    hk::vector<ResourceMetadata> buffer_metas;
    hk::vector<ResourceMetadata> image_metas;

    hk::vector<BufferDesc> buffer_descs;
    hk::vector<ImageDesc> image_descs;

    u32 buffer_count;
    u32 image_count;

    // std::deque<Handle> deletion_queue;

    // Shaders
    // hk::vector<VkShaderModule> shaders;

    // Pipelines
    // hk::vector<VkPipeline> pipelines;
    // hk::vector<VkPipelineLayout> pipeline_layouts;

    // Descriptors?

    VkDevice device = VK_NULL_HANDLE;
} ctx;

void init()
{
    ctx.device = hk::vkc::device();

    constexpr u32 initial_size = 2048;
    ctx.buffer_pool.resize(initial_size);
    ctx.buffer_metas.resize(initial_size);
    ctx.buffer_descs.resize(initial_size);
    ctx.image_pool.resize(initial_size);
    ctx.image_metas.resize(initial_size);
    ctx.image_descs.resize(initial_size);

    ctx.free_buffers.resize(initial_size);
    for (u32 i = initial_size - 1; i > 0; --i) {
        ctx.free_buffers.push_back(i);
    }
    ctx.free_buffers.push_back(0);

    ctx.free_images.resize(initial_size);
    for (u32 i = initial_size - 1; i > 0; --i) {
        ctx.free_images.push_back(i);
    }
    ctx.free_images.push_back(0);

    ctx.buffer_count = 0;
    ctx.image_count = 0;
}

void deinit()
{
    // TODO: Destroy all resources
}

u32 find_memory_idx(VkMemoryRequirements requirements, VkMemoryPropertyFlags properties)
{
    auto mem_properties = hk::vkc::adapter_info().memory_properties;

    u32 mem_index = 0;
    for (; mem_index < mem_properties.memoryTypeCount; ++mem_index) {
        if ((requirements.memoryTypeBits & (1 << mem_index)) &&
            (mem_properties.memoryTypes[mem_index].propertyFlags &
            properties) == properties)
        {
            break;
        }
    }

    return mem_index;
}

InternalBuffer allocate_buffer(const BufferDesc &desc)
{
    VkResult err;

    InternalBuffer buffer;

    VkBufferCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    info.size = desc.size * desc.stride;
    info.usage = to_vulkan(desc.type);
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (desc.access == MemoryType::CPU_UPLOAD) {
        info.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    } else {
        info.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    }

    err = vkCreateBuffer(ctx.device, &info, nullptr, &buffer.handle);
    ALWAYS_ASSERT(!err, "Failed to create Vulkan Buffer");

    VkMemoryRequirements mem_requirements;
    vkGetBufferMemoryRequirements(ctx.device, buffer.handle, &mem_requirements);

    VkMemoryPropertyFlags properties = to_vulkan(desc.access);

    VkMemoryAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_requirements.size;
    alloc_info.memoryTypeIndex = find_memory_idx(mem_requirements, properties);

    err = vkAllocateMemory(ctx.device, &alloc_info, nullptr, &buffer.memory);
    ALWAYS_ASSERT(!err, "Failed to allocate Vulkan Buffer Memory");

    vkBindBufferMemory(ctx.device, buffer.handle, buffer.memory, 0);

    return buffer;
}

void deallocate_buffer(InternalBuffer &buffer)
{
    // TODO: change to 'b8 in_use'
    if (ctx.device) { vkDeviceWaitIdle(ctx.device); }

    // buffer should be valide at this point, no need for checks

    vkDestroyBuffer(ctx.device, buffer.handle, nullptr);
    vkFreeMemory(ctx.device, buffer.memory, nullptr);
}

// TODO: Move to utils
constexpr u64 align_size(u64 size, u64 alignment)
{
    // https://github.com/SaschaWillems/Vulkan/blob/master/examples/dynamicuniformbuffer/dynamicuniformbuffer.cpp#L297
    return (alignment == 0) ? size : (size + alignment - 1) & ~(alignment - 1);
}

BufferHandle create_buffer(const BufferDesc &desc, const std::string &name)
{
    auto limits = hk::vkc::adapter_info().properties.limits;

    u32 size = desc.size;

    u32 stride = (desc.type != BufferType::UNIFORM_BUFFER) ?
        desc.stride : align_size(desc.stride, limits.minUniformBufferOffsetAlignment);

    BufferDesc buffer_desc = {
        desc.type,
        desc.access,
        size,
        stride
    };

    InternalBuffer buffer = allocate_buffer(buffer_desc);

    u32 idx = ctx.free_buffers.back();
    ctx.free_buffers.pop_back();

    auto &slot = ctx.buffer_pool.at(idx);
    slot.data = buffer;
    slot.is_valid = true;

    BufferHandle handle = { idx, slot.gen };

    ResourceMetadata meta;
    meta.name = name;
    meta.handle.value = handle.value;

    ctx.buffer_metas.at(idx) = meta;
    ctx.buffer_descs.at(idx) = buffer_desc;

    ++ctx.buffer_count;

    hk::debug::setName(buffer.handle, "Buffer - "        + name);
    hk::debug::setName(buffer.memory, "Buffer Memory - " + name);

    return handle;
}

void destroy_buffer(const BufferHandle &handle)
{
    // TODO: delete desc, meta, mark index free, add checks, etc

    auto &slot = ctx.buffer_pool.at(handle.index);
    ALWAYS_ASSERT(handle.gen == slot.gen);
    ALWAYS_ASSERT(slot.is_valid);

    // TODO: push to dealloc queue
    deallocate_buffer(slot.data);

    ++slot.gen;
    slot.is_valid = false;

    ctx.free_buffers.push_back(handle.index);

    --ctx.buffer_count;
}

void resize_buffer(const BufferHandle &handle, u32 size)
{
    BufferDesc &desc = ctx.buffer_descs.at(handle.index);
    desc.size = size;

    auto &slot = ctx.buffer_pool.at(handle.index);
    ALWAYS_ASSERT(handle.gen == slot.gen);
    ALWAYS_ASSERT(slot.is_valid);

    deallocate_buffer(slot.data);
    InternalBuffer buffer = allocate_buffer(desc);
    slot.data = buffer;
}

void update_buffer(const BufferHandle &handle, const void *data)
{
    auto &slot = ctx.buffer_pool.at(handle.index);
    ALWAYS_ASSERT(handle.gen == slot.gen);
    ALWAYS_ASSERT(slot.is_valid);

    BufferDesc &desc = ctx.buffer_descs.at(handle.index);

    u32 memsize = desc.size * desc.stride;

    if (desc.access == MemoryType::CPU_UPLOAD) {
        void *mapped = nullptr;

        vkMapMemory(ctx.device, slot.data.memory, 0, memsize, 0, &mapped);
        std::memcpy(mapped, data, memsize);
        vkUnmapMemory(ctx.device, slot.data.memory);

        return;
    }

    BufferDesc staging_desk = {
        BufferType::NONE,
        MemoryType::CPU_UPLOAD,
        desc.size,
        desc.stride
    };

    BufferHandle staging = create_buffer(staging_desk);
    update_buffer(staging, data);

    hk::vkc::submitImmCmd([&](VkCommandBuffer cmd) {
        VkBufferCopy copyRegion = {};
        copyRegion.size = memsize;
        vkCmdCopyBuffer(cmd, ctx.buffer_pool.at(staging.index).data.handle, slot.data.handle, 1, &copyRegion);
    });

    destroy_buffer(staging);
}

void bind_buffer(const BufferHandle &handle, VkCommandBuffer cmd)
{
    auto &slot = ctx.buffer_pool.at(handle.index);
    BufferDesc &desc = ctx.buffer_descs.at(handle.index);

    switch(desc.type) {
    case BufferType::VERTEX_BUFFER: {
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(cmd, 0, 1, &slot.data.handle, offsets);
    } break;

    case BufferType::INDEX_BUFFER: {
        vkCmdBindIndexBuffer(cmd, slot.data.handle, 0, VK_INDEX_TYPE_UINT32);
    } break;

    default: { return; }
    }
}

// TODO: add checks
const BufferDesc& desc(const BufferHandle &handle)
{
    return ctx.buffer_descs.at(handle.index);
}
const ResourceMetadata& meta(const BufferHandle &handle)
{
    return ctx.buffer_metas.at(handle.index);
}

VkBuffer handle(BufferHandle handle)
{
    return ctx.buffer_pool.at(handle.index).data.handle;
}

HKAPI const hk::vector<ResourceMetadata>& metadata() { return ctx.buffer_metas; }
HKAPI const hk::vector<BufferDesc>& descriptors() { return ctx.buffer_descs; }
HKAPI const u32 size() { return ctx.buffer_count; };

/* ===== Images ===== */
constexpr VkImageAspectFlags aspect_mask(const ImageDesc &desc)
{
    VkImageAspectFlags out = VK_IMAGE_ASPECT_NONE;

    if (desc.type == ImageType::DEPTH_BUFFER) {
        out = VK_IMAGE_ASPECT_DEPTH_BIT;

        switch (desc.format) {
        case hk::Format::D32_SFLOAT_S8_UINT:
        case hk::Format::D24_UNORM_S8_UINT: {
            out |= VK_IMAGE_ASPECT_STENCIL_BIT;
        } break;

        default: { break; }
        }
    } else {
        out = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    return out;
}

// Helpers for layout transition
constexpr VkPipelineStageFlags pipeline_stage(VkImageLayout layout, b8 src)
{
    VkPipelineStageFlags out = VK_PIPELINE_STAGE_NONE;

    switch (layout) {
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL: {
        out = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } break;
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL: {
        out = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } break;
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL: {
        out =
            VK_PIPELINE_STAGE_VERTEX_SHADER_BIT   |
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    } break;
    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: {
        out = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    } break;
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL: {
        out =
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
            VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    } break;
    case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR: {
        out = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    } break;
    case VK_IMAGE_LAYOUT_GENERAL: {
        out = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    } break;
    default: {
        if (src) {
            out = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;  // src
        } else {
            out = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT; // dst
        }
    }
    }

    return out;
}

constexpr VkAccessFlags access(VkImageLayout layout, b8 src)
{
    VkAccessFlags out = VK_ACCESS_NONE;

    switch (layout) {
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL: {
        out = VK_ACCESS_TRANSFER_READ_BIT;
    } break;
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL: {
        out = VK_ACCESS_TRANSFER_WRITE_BIT;
    } break;
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL: {
        out = VK_ACCESS_SHADER_READ_BIT;
    } break;
    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: {
        out = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        if (!src) {
            out |= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT; // dst
        }
    } break;
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL: {
        out = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        if (!src) {
            out |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT; // dst
        }
    } break;
    case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR: {
        out = VK_ACCESS_MEMORY_READ_BIT;
    } break;
    case VK_IMAGE_LAYOUT_GENERAL: {
        out = VK_ACCESS_HOST_READ_BIT | VK_ACCESS_HOST_WRITE_BIT;
    } break;
    default: {
        out = VK_ACCESS_NONE;
    }
    }

    return out;
}

constexpr VulkanImageDesc vulkan_desc(const ImageDesc &desc)
{
    VulkanImageDesc vkdesc = {};

    vkdesc.width = desc.width;
    vkdesc.height = desc.height;

    vkdesc.format = to_vulkan(desc.format);

    vkdesc.usage = to_vulkan(desc.type);
    vkdesc.aspect_mask = aspect_mask(desc);

    vkdesc.tiling = VK_IMAGE_TILING_OPTIMAL;
    vkdesc.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    // FIX: temp
    vkdesc.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    vkdesc.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    return vkdesc;
}

InternalImage allocate_image(const VulkanImageDesc &desc)
{
    VkResult err;

    InternalImage image;

    VkImageCreateInfo image_info = {};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.extent.width = desc.width;
    image_info.extent.height = desc.height;
    image_info.extent.depth = 1;
    image_info.mipLevels = 1;
    image_info.arrayLayers = 1;
    image_info.format = desc.format;
    image_info.tiling = desc.tiling;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.usage = desc.usage;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    err = vkCreateImage(ctx.device, &image_info, nullptr, &image.handle);
    ALWAYS_ASSERT(!err, "Failed to create Vulkan Image");

    VkMemoryRequirements mem_requirements;
    vkGetImageMemoryRequirements(ctx.device, image.handle, &mem_requirements);

    auto mem_properties = hk::vkc::adapter_info().memory_properties;

    VkMemoryPropertyFlags properties = desc.properties;

    VkMemoryAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_requirements.size;
    alloc_info.memoryTypeIndex = find_memory_idx(mem_requirements, properties);

    err = vkAllocateMemory(ctx.device, &alloc_info, nullptr, &image.memory);
    ALWAYS_ASSERT(!err, "Failed to allocate memory for Vulkan Image");

    vkBindImageMemory(ctx.device, image.handle, image.memory, 0);

    VkImageViewCreateInfo view_info = {};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image = image.handle;
    view_info.format = desc.format;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.subresourceRange.aspectMask = desc.aspect_mask;

    // TODO: config
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = 1;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = 1;

    view_info.components.r = VK_COMPONENT_SWIZZLE_R;
    view_info.components.g = VK_COMPONENT_SWIZZLE_G;
    view_info.components.b = VK_COMPONENT_SWIZZLE_B;
    view_info.components.a = VK_COMPONENT_SWIZZLE_A;

    err = vkCreateImageView(ctx.device, &view_info, nullptr, &image.view);
    ALWAYS_ASSERT(!err, "Failed to create Vulkan Image View");

    return image;
}

void deallocate_image(InternalImage &image)
{
    // TODO: change to 'b8 in_use'
    if (ctx.device) { vkDeviceWaitIdle(ctx.device); }

    // image should be valide at this point, no need for checks

    vkDestroyImageView(ctx.device, image.view, nullptr);
    vkDestroyImage(ctx.device, image.handle, nullptr);
    vkFreeMemory(ctx.device, image.memory, nullptr);
}

ImageHandle create_image(const ImageDesc &desc, const std::string &name)
{
    ImageDesc image_desc = {
        desc.type,
        desc.format,
        desc.width,
        desc.height,
        desc.channels,
        { VK_IMAGE_LAYOUT_UNDEFINED }
    };
    // desc.layout_history.push_back({ VK_IMAGE_LAYOUT_UNDEFINED });

    InternalImage image = allocate_image(vulkan_desc(image_desc));

    u32 idx = ctx.free_images.back();
    ctx.free_images.pop_back();

    auto &slot = ctx.image_pool.at(idx);
    slot.data = image;
    slot.is_valid = true;

    ImageHandle handle = { idx, slot.gen };

    ResourceMetadata meta;
    meta.name = name;
    meta.handle.value = handle.value;

    ctx.image_metas.at(idx) = meta;
    ctx.image_descs.at(idx) = image_desc;

    ++ctx.image_count;

    VkImageLayout layout;
    switch (desc.type) {
    case ImageType::TEXTURE:
        layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; break;
    case ImageType::RENDER_TARGET:
        layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; break;
    case ImageType::DEPTH_BUFFER:
        layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL; break;
    default: { break; }
    }

    transition_image_layout(handle, layout);

    hk::debug::setName(image.handle, "Image - "        + name);
    hk::debug::setName(image.view,   "Image View - "   + name);
    hk::debug::setName(image.memory, "Image Memory - " + name);

    return handle;
}

void destroy_image(const ImageHandle &handle)
{
    // TODO: delete desc, meta, mark index free, add checks, etc

    auto &slot = ctx.image_pool.at(handle.index);
    ALWAYS_ASSERT(handle.gen == slot.gen);
    ALWAYS_ASSERT(slot.is_valid);

    // TODO: don't dealloc on a spot, mark for deletion and
    // clear after sure it's not used anymore
    deallocate_image(slot.data);

    ++slot.gen;
    slot.is_valid = false;

    ctx.free_images.push_back(handle.index);

    --ctx.image_count;
}

void write_image(const ImageHandle &handle, const void *pixels)
{
    auto &slot = ctx.image_pool.at(handle.index);
    ALWAYS_ASSERT(handle.gen == slot.gen);
    ALWAYS_ASSERT(slot.is_valid);

    ImageDesc &desc = ctx.image_descs.at(handle.index);

    u32 size = desc.width * desc.height * 4; // Assuming 4 bytes per pixel

    BufferDesc staging_desc = {
        BufferType::NONE,
        MemoryType::CPU_UPLOAD,
        size, 1
    };

    BufferHandle staging_buf = bkr::create_buffer(staging_desc);

    bkr::update_buffer(staging_buf, pixels);

    VkImageLayout old_layout = desc.layout_history.back();

    transition_image_layout(handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    // Copy from buffer to image
    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    // region.bufferRowLength = 0;
    // region.bufferImageHeight = 0;
    region.bufferRowLength = desc.width;
    region.bufferImageHeight = desc.height;
    region.imageSubresource.aspectMask = aspect_mask(desc);
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = { desc.width, desc.height, 1 };

    hk::vkc::submitImmCmd([&](VkCommandBuffer cmd) {
        vkCmdCopyBufferToImage(
            cmd,
            bkr::handle(staging_buf),
            slot.data.handle,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &region);
    });

    transition_image_layout(handle, old_layout);

    bkr::destroy_buffer(staging_buf);
}

void copy_image(const ImageHandle &src, const ImageHandle &dst)
{
    auto &src_slot = ctx.image_pool.at(src.index);
    ALWAYS_ASSERT(src.gen == src_slot.gen);
    ALWAYS_ASSERT(src_slot.is_valid);

    auto &dst_slot = ctx.image_pool.at(dst.index);
    ALWAYS_ASSERT(dst.gen == dst_slot.gen);
    ALWAYS_ASSERT(dst_slot.is_valid);

    ImageDesc &src_desc = ctx.image_descs.at(src.index);
    ImageDesc &dst_desc = ctx.image_descs.at(dst.index);

    ALWAYS_ASSERT(src_desc.width == dst_desc.width);
    ALWAYS_ASSERT(src_desc.height == dst_desc.height);

    VkImageLayout src_old_layout = src_desc.layout_history.back();
    transition_image_layout(src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

    VkImageLayout dst_old_layout = dst_desc.layout_history.back();
    transition_image_layout(dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    VkImageCopy copy_region = {};
    copy_region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copy_region.srcSubresource.mipLevel = 0;
    copy_region.srcSubresource.baseArrayLayer = 0;
    copy_region.srcSubresource.layerCount = 1;
    copy_region.dstSubresource.aspectMask = aspect_mask(src_desc);
    copy_region.dstSubresource.mipLevel = 0;
    copy_region.dstSubresource.baseArrayLayer = 0;
    copy_region.dstSubresource.layerCount = 1;
    copy_region.extent.width = src_desc.width;
    copy_region.extent.height = src_desc.height;
    copy_region.extent.depth = 1;

    hk::vkc::submitImmCmd([&](VkCommandBuffer cmd) {
        vkCmdCopyImage(
            cmd,
            src_slot.data.handle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            dst_slot.data.handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &copy_region
        );
    });

    transition_image_layout(src, src_old_layout);
    transition_image_layout(dst, dst_old_layout);
}

void transition_image_layout(const ImageHandle &handle, VkImageLayout target)
{
    auto &slot = ctx.image_pool.at(handle.index);
    ALWAYS_ASSERT(handle.gen == slot.gen);
    ALWAYS_ASSERT(slot.is_valid);

    ALWAYS_ASSERT(target != VK_IMAGE_LAYOUT_UNDEFINED,
                  "Can't transition to undefined layout");

    ImageDesc &desc = ctx.image_descs.at(handle.index);
    VkImageLayout layout = desc.layout_history.back();

    if (layout == target) { return; }

    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = layout;
    barrier.newLayout = target;
    barrier.srcQueueFamilyIndex = hk::vkc::graphics().family();
    barrier.dstQueueFamilyIndex = hk::vkc::graphics().family();
    barrier.image = slot.data.handle;

    barrier.subresourceRange.aspectMask = aspect_mask(desc);

    // TODO: config
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    barrier.srcAccessMask = access(layout, true);
    barrier.dstAccessMask = access(target, false);

    VkPipelineStageFlags src_stage = pipeline_stage(layout, true);
    VkPipelineStageFlags dst_stage = pipeline_stage(target, false);

    hk::vkc::submitImmCmd([&](VkCommandBuffer cmd) {
        vkCmdPipelineBarrier(
            cmd,
            src_stage, dst_stage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );
    });

    desc.layout_history.push_back(target);
}

const ImageDesc& desc(ImageHandle handle)
{
    return ctx.image_descs.at(handle.index);
}

const ResourceMetadata& meta(ImageHandle handle)
{
    return ctx.image_metas.at(handle.index);
}

VkImage image(ImageHandle handle)
{
    return ctx.image_pool.at(handle.index).data.handle;
}

VkImageView view(ImageHandle handle)
{
    return ctx.image_pool.at(handle.index).data.view;
}

const hk::vector<ResourceMetadata>& image_metadata()
{
    return ctx.image_metas;
}

const hk::vector<ImageDesc>& image_descriptors()
{
    return ctx.image_descs;
}

const u32 image_size()
{
    return ctx.image_count;
}

// returns handle
// u32 loadShader(hk::dxc::ShaderDesc desc)
// {
//     const hk::vector<u32> spirv = hk::dxc::loadShader(desc);
//
//     if (spirv.size() <= 0) {
//         LOG_WARN("Shader code is empty");
//         // TODO: return default shader
//         return 0;
//     }
//
//     Shader shader;
//     // asset->handle = index_++;
//
//     shader.meta.name = desc.path.substr(desc.path.find_last_of("/\\") + 1);
//     shader.meta.desc = desc;
//
//     // Create Shader Module
//     VkResult err;
//
//     VkShaderModuleCreateInfo info = {};
//     info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
//     info.codeSize = spirv.size() * sizeof(u32);
//     info.pCode = spirv.data();
//
//     err = vkCreateShaderModule(ctx.device, &info, nullptr, &shader.module);
//     ALWAYS_ASSERT(!err, "Failed to create Vulkan Shader Module");
//
//     hk::debug::setName(shader.module, shader.meta.name);
//
//     ctx.shaders.push_back(shader);
//
//     // return shader.handle;
//     return ctx.shaders.size();
// }
//
// void unloadShader(u32 handle)
// {
//     hk::Shader shader = ctx.shaders.at(handle);
//
//     vkDestroyShaderModule(hk::vkcontext::device(), shader.module, nullptr);
//
//     ctx.shaders.erase(handle);
// }
//
// void reloadShader(u32 handle)
// {
//     hk::Shader shader = ctx.shaders.at(handle);
//     vkDestroyShaderModule(hk::vkcontext::device(), shader.module, nullptr);
//
//     const hk::vector<u32> spirv = hk::dxc::loadShader(shader.meta.desc);
//
//     if (spirv.size() <= 0) {
//         LOG_WARN("Shader code was not reloaded");
//         return;
//     }
//
//     // Create Shader Module
//     VkResult err;
//
//     VkShaderModuleCreateInfo info = {};
//     info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
//     info.codeSize = spirv.size() * sizeof(u32);
//     info.pCode = spirv.data();
//
//     err = vkCreateShaderModule(ctx.device, &info, nullptr, &shader.module);
//     ALWAYS_ASSERT(!err, "Failed to create Vulkan Shader Module");
//
//     hk::debug::setName(shader.module, shader.meta.name);
// }

}
