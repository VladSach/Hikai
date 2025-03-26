#include "globals.hlsli"

[[vk::push_constant]]
struct ShapeDesc {
    float3 color;

    float psize;
    bool use_depth;
} desc;

struct VertexOutput {
    float4 pos : SV_Position;
    // https://github.com/Microsoft/DirectXShaderCompiler/blob/main/docs/SPIR-V.rst#builtin-variables
    [[vk::builtin("PointSize")]] float psize : PSIZE;
};

VertexOutput main(float3 pos : POSITION)
{
    VertexOutput output;
    output.pos = mul(camera.view_proj, float4(pos, 1.f));
    output.psize = desc.psize;
    return output;
}
