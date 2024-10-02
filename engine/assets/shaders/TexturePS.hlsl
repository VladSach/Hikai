#include "globals.hlsli"

[[vk::combinedImageSampler]]
Texture2D<float4> myTexture : register(t1, space1);
[[vk::combinedImageSampler]]
SamplerState mySampler : register(s1, space1);

struct PixelInput {
    float4 pos : SV_Position;
    float3 normal : NORMAL;
    float2 tc : TEXCOORD0;
};

float4 main(PixelInput input) : SV_Target0
{
    return myTexture.Sample(mySampler, input.tc);
}
