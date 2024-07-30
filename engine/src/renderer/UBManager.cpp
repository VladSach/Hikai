#include "UBManager.h"

#include "renderer/Descriptors.h"

#include "renderer/vkwrappers/Buffer.h"

// FIX: temp
static VkDescriptorSet sceneDataDescriptor = VK_NULL_HANDLE;
static VkDescriptorSetLayout tmpDescLayout = VK_NULL_HANDLE;
static hk::DescriptorWriter *globalDsWriter;
VkDescriptorSet *getGlobalDescriptorSet()
{
    return &sceneDataDescriptor;
}
VkDescriptorSetLayout getGlobalDescriptorSetLayout()
{
    return tmpDescLayout;
}
hk::DescriptorWriter *getGlobalDescriptorWriter()
{
    return globalDsWriter;
}

namespace hk::ubo {

static SceneData frameData;
static Buffer frameDataBuffer;

void init()
{
    Buffer::BufferDesc uniformDisc;
    uniformDisc.type = Buffer::Type::UNIFORM_BUFFER;
    uniformDisc.usage = Buffer::Usage::NONE;
    uniformDisc.property = Buffer::Property::CPU_ACESSIBLE;
    uniformDisc.size = 2; // FIX: temp, make depend on framebuffers size
    uniformDisc.stride = sizeof(frameData);

    frameDataBuffer.init(uniformDisc);

    // FIX: all this
    // DescriptorLayout sceneDataDescriptorLayout =
    //     DescriptorLayout::Builder()
    //     .addBinding(0,
    //                 VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    //                 VK_SHADER_STAGE_ALL_GRAPHICS)
    //     .build();

    DescriptorLayout *sceneDataDescriptorLayout =
        new DescriptorLayout(DescriptorLayout::Builder()
        .addBinding(0,
                    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    VK_SHADER_STAGE_ALL_GRAPHICS)
        .addBinding(1,
                    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    VK_SHADER_STAGE_FRAGMENT_BIT)
        .build()
    );

    // FIX: tmp
    tmpDescLayout = sceneDataDescriptorLayout->layout();
    globalDsWriter = new hk::DescriptorWriter();

    sceneDataDescriptor =
        // hk::pool()->allocate(sceneDataDescriptorLayout.layout());
        hk::pool()->allocate(sceneDataDescriptorLayout->layout());
}

void deinit()
{
    frameDataBuffer.deinit();
}

void setFrameData(const SceneData &ubo)
{
    frameData = ubo;

    frameDataBuffer.update(&frameData);

    // hk::DescriptorWriter writer;
    globalDsWriter->writeBuffer(0, frameDataBuffer.buffer(),
                                sizeof(SceneData), 0,
                                VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    // globalDsWriter->updateSet(sceneDataDescriptor);
}

}
