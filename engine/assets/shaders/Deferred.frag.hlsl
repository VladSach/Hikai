cbuffer MaterialConstants : register(b0, space2) {
    float4 color;
    float4 specular;
    float4 ambient;

    float opacity;

    float metalness;
    float roughness;
};

[[vk::combinedImageSampler]]
Texture2D<float4> diffuseMap : register(t1, space2);
[[vk::combinedImageSampler]]
SamplerState diffuseSampler : register(s1, space2);

[[vk::combinedImageSampler]]
Texture2D<float4> normalMap : register(t2, space2);
[[vk::combinedImageSampler]]
SamplerState normalSampler : register(s2, space2);

[[vk::combinedImageSampler]]
Texture2D<float4> emissiveMap : register(t3, space2);
[[vk::combinedImageSampler]]
SamplerState emissiveSampler : register(s3, space2);

[[vk::combinedImageSampler]]
Texture2D<float4> metalnessMap : register(t4, space2);
[[vk::combinedImageSampler]]
SamplerState metalnessSampler : register(s4, space2);

[[vk::combinedImageSampler]]
Texture2D<float4> roughnessMap : register(t5, space2);
[[vk::combinedImageSampler]]
SamplerState roughnessSampler : register(s5, space2);

[[vk::combinedImageSampler]]
Texture2D<float4> lightMap : register(t6, space2);
[[vk::combinedImageSampler]]
SamplerState lightmapSampler : register(s6, space2);

struct PixelInput {
    float4 sv_pos : SV_Position;
    float3 normal : NORMAL;
    float2 tc : TEXCOORD0;
    float3 pos : POSITION0;
    float3 tangent : TANGENT;
};

struct PixelOutput
{
    float4 position: SV_Target0;
    float4 normal  : SV_Target1;
    float4 albedo  : SV_Target2;
    float4 material: SV_Target3;
    float4 color   : SV_Target4;
};

PixelOutput main(PixelInput input)
{
    PixelOutput output;

    float4 albedo;
    float3 normal;
    float metallic;
    float rough;
    float ao;

    float4 diffuse_value = diffuseMap.Sample(diffuseSampler, input.tc);
    float3 normal_value = normalMap.Sample(normalSampler, input.tc).rgb;
    float metallic_value = metalnessMap.Sample(metalnessSampler, input.tc).r;
    float roughness_value = roughnessMap.Sample(roughnessSampler, input.tc).r;
    float ao_value = lightMap.Sample(lightmapSampler, input.tc).r;

    albedo = lerp(float4(color.rgb, 1.f), diffuse_value, any(diffuse_value != (.0f).xxxx));
    normal = lerp(input.normal, normal_value, any(normal_value != (.0f).xxx));
    metallic = lerp(metalness, metalness * metallic_value, any(metallic_value != .0f));
    rough = lerp(roughness, roughness * roughness_value, any(roughness_value != .0f));
    ao = lerp(1, ao_value, any(ao_value != .0f));

    if (!all(normal == input.normal)) {
        float3 B = cross(input.normal, input.tangent);
        float3x3 TBN = float3x3(input.tangent, B, input.normal);
        normal = normalize(mul(TBN, normalize(normal * 2.f - 1.f)));
    }

    output.position = float4(input.pos, 1.f);
    output.normal = float4(normal, 1.f);
    output.albedo = albedo;
    output.material.r = metallic;
    output.material.g = rough;
    output.material.b = ao;
    output.material.a = 1.f;

    // Write to avoid undefined behaviour (validation error)
    output.color = 0.f;
    return output;
}
