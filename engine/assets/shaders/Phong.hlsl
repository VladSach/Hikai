#include "globals.hlsli"

[[vk::combinedImageSampler]]
Texture2D<float4> myTexture : register(t1, space1);
[[vk::combinedImageSampler]]
SamplerState mySampler : register(s1, space1);

cbuffer MaterialConstants : register(b0, space1) {
    float4 color;
    float4 emissive;
    float alpha;
    float shininess;
    float reflectivity;
};

struct PixelInput {
    float4 pos : SV_Position;
    float3 normal : NORMAL;
    float2 tc : TEXCOORD0;
    float3 world : POSITION0;
};

float4 main(PixelInput input) : SV_Target0
{
    float4 tx = myTexture.Sample(mySampler, input.tc);
    return tx;

    float3 light_color = float3(0.1f, 0.8f, 0.3f); // all
    float3 light_position = float3(0.5f, 0.0f, -1.0f); // direct
    //float3 light_color = lights[0].color.xyz; // all
    //float3 light_position = lights[0].position; // direct

    float ambient_strength = 0.1f;
    float3 ambient = ambient_strength * light_color;

    float3 light_dir = normalize(light_position - input.world);
    float diff = max(dot(input.normal, light_dir), 0.0);
    float3 diffuse = diff * light_color;

    float specular_strength = 0.5f;
    float3 view_dir = normalize(cameraPos - input.world);
    float3 reflect_dir = reflect(light_dir, input.normal);
    float spec = pow(max(dot(view_dir, reflect_dir), .0f), 32);
    float3 specular = specular_strength * spec * light_color;

    return color * float4(ambient + diffuse + specular, 1.f);
}
