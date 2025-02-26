[[vk::combinedImageSampler]]
Texture2D<float4> presentTexture : register(t2, space0);
[[vk::combinedImageSampler]]
SamplerState presentSampler : register(s2, space0);

struct PixelInput {
    float4 pos : SV_Position;
    float2 tc : TEXCOORD0;
};

float4 main(PixelInput input) : SV_Target0
{
    return presentTexture.Sample(presentSampler, input.tc);
}
