#include "UBManager.h"

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
}

void deinit()
{
    frameDataBuffer.deinit();
}

Buffer& getFrameData()
{
    return frameDataBuffer;
}

void setFrameData(const SceneData &ubo)
{
    frameData = ubo;

    frameDataBuffer.update(&frameData);
}

}
