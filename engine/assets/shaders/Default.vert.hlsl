#include "globals.hlsli"
#include "utils.hlsli"

struct VertexInput {
    float3 pos : POSITION0;
    float3 normal : COLOR0;
    float2 tc : TEXCOORD0;
    float3 tangent : TANGENT;
};

struct VertexOutput {
    float4 position : SV_Position;
    float3 normal : NORMAL;
    float2 tc : TEXCOORD0;
    float3 world : POSITION0;
    float3 tangent : TANGENT;
};

[[vk::push_constant]]
struct ModelToWorld {
    float4x4 mat;
} modelToWorld;

VertexOutput main(VertexInput input) {
    VertexOutput output;

    float4 world_pos = mul(modelToWorld.mat, float4(input.pos, 1.f));
    output.position = mul(viewProj, world_pos);

    float4x4 inverse_model = transpose(inverse(modelToWorld.mat));
    float3 tangent = normalize(mul(inverse_model, float4(input.tangent, 0.f)).xyz);

    output.normal = normalize(mul(inverse_model, float4(input.normal, 0.f)).xyz);
    output.tangent = normalize(tangent - dot(tangent.xyz, output.normal) * output.normal.xyz);

    output.tc = input.tc;
    output.world = world_pos.xyz;

    return output;
}
