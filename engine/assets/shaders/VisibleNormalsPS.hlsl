cbuffer PerFrame : register(b0) {
    float2 resolution;
    float time;
};

[[vk::combinedImageSampler]]
Texture2D<float4> myTexture : register(t1);
[[vk::combinedImageSampler]]
SamplerState mySampler : register(s1);
//return myTexture.Sample(mySampler, input.tc);

struct PixelInput {
    float4 pos : SV_Position;
    float3 normal : NORMAL;
    float2 tc : TEXCOORD0;
};


float4 main(PixelInput input) : SV_Target0
{
    float3 norm = ((input.normal + 1) * .5f);
    return float4(norm, 1.f);

}
