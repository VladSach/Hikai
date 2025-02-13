#include "globals.hlsli"

struct VertexInput {
    float3 pos : POSITION0;
    float3 normal : COLOR0;
    float2 tc : TEXCOORD0;
};

struct VertexOutput {
    float4 position : SV_Position;
    float3 normal : NORMAL;
    float2 tc : TEXCOORD0;
    float3 world : POSITION0;
};

VertexOutput main(VertexInput input) {
    VertexOutput output;

    float4 world_pos = mul(modelToWorld.mat, float4(input.pos, 1.f));
    output.position = mul(viewProj, world_pos);
    output.normal = input.normal;
    output.tc = input.tc;
    output.world = world_pos;

    return output;
}
