#include "globals.hlsli"
#include "pbr.hlsli"

cbuffer MaterialConstants : register(b0, space1) {
    float4 color;
    float4 specular;
    float4 ambient;

    float opacity;

    float metalness;
    float roughness;
};

[[vk::combinedImageSampler]]
Texture2D<float4> diffuseMap : register(t1, space1);
[[vk::combinedImageSampler]]
SamplerState diffuseSampler : register(s1, space1);
[[vk::combinedImageSampler]]
Texture2D<float4> normalMap : register(t2, space1);
[[vk::combinedImageSampler]]
SamplerState normalSampler : register(s2, space1);
[[vk::combinedImageSampler]]
Texture2D<float4> emissiveMap : register(t3, space1);
[[vk::combinedImageSampler]]
SamplerState emissiveSampler : register(s3, space1);
[[vk::combinedImageSampler]]
Texture2D<float4> metalnessMap : register(t4, space1);
[[vk::combinedImageSampler]]
SamplerState metalnessSampler : register(s4, space1);
[[vk::combinedImageSampler]]
Texture2D<float4> roughnessMap : register(t5, space1);
[[vk::combinedImageSampler]]
SamplerState roughnessSampler : register(s5, space1);
[[vk::combinedImageSampler]]
Texture2D<float4> lightMap : register(t6, space1);
[[vk::combinedImageSampler]]
SamplerState lightmapSampler : register(s6, space1);

struct PixelInput {
    float4 sv_pos : SV_Position;
    float3 normal : NORMAL;
    float2 tc : TEXCOORD0;
    float3 pos : POSITION0;
    float3 tangent : TANGENT;
};

float4 calculateDirectionalLight(float3 normal, DirectionalLight light)
{
    float3 lightDir = normalize(-light.direction);
    float NdotL = saturate(dot(normal, lightDir));

    float3 diffuse = light.color * NdotL;
    return float4(diffuse, 1.0f);
}

float4 calculateSpotLight(PixelInput input, SpotLight light)
{
    float3 lightDir = normalize(light.position - input.pos);

    // Calculate the cone of influence
    float lightAngle = dot(normalize(light.direction), -lightDir);

    if (acos(lightAngle) > radians(light.outer_cutoff) || lightAngle < 0)
        return float4(0, 0, 0, 0);

    float distance = length(light.position - input.pos);
    float intensity = 1.f / (distance * distance);

    if (acos(lightAngle) > radians(light.inner_cutoff)) {
        float t = (acos(lightAngle) - radians(light.outer_cutoff)) /
                  (radians(light.inner_cutoff) - radians(light.outer_cutoff));
        intensity *= t;
    }

    intensity = saturate(intensity);

    if (intensity == 0) return float4(0, 0, 0, 0);

    float3 viewDir = normalize(cameraPos - input.pos);
    float3 halfwayDir = normalize(lightDir + viewDir);

    float diffuse = max(dot(input.normal, lightDir), 0.0001f);
    float specular = pow(max(dot(input.normal, halfwayDir), 0.0001f), 2.f);

    return float4(light.color) * intensity * (diffuse + specular);
}

float4 calculatePointLight(PixelInput input, PointLight light)
{
    float3 lightDir = normalize(light.position - input.pos);
    float3 viewDir = normalize(cameraPos - input.pos);
    float3 halfwayDir = normalize(lightDir + viewDir);

    float diffuse = max(dot(input.normal, lightDir), 0.0001f);
    float specular = pow(max(dot(input.normal, halfwayDir), 0.0001f), 2.f);

    float distance = length(light.position - input.pos);
    float intensity = 1.f / (distance * distance);

    return float4(light.color) * intensity * (diffuse + specular);
}

float4 main(PixelInput input) : SV_Target0
{
    float4 albedo;
    float3 normal;
    float metallic;
    float r; // roughness

    float4 diffuse_value = diffuseMap.Sample(diffuseSampler, input.tc);
    float3 normal_value = normalMap.Sample(normalSampler, input.tc).rgb;
    float metallic_value = metalnessMap.Sample(metalnessSampler, input.tc).r;
    float roughness_value = roughnessMap.Sample(roughnessSampler, input.tc).r;

    albedo = lerp(float4(color.rgb, 1.f), diffuse_value, any(diffuse_value != (.0f).xxxx));
    normal = lerp(input.normal, normal_value, any(normal_value != (.0f).xxx));
    metallic = lerp(metalness, metalness * metallic_value, any(metallic_value != .0f));
    r        = lerp(roughness, roughness * roughness_value, any(roughness_value != .0f));

    albedo = pow(albedo, 2.2f);

    float3 B = cross(input.normal, input.tangent);
    float3x3 TBN = float3x3(input.tangent, B, input.normal);
    if (!all(normal == input.normal)) {
        normal = normalize(mul(TBN, normalize(normal * 2.f - 1.f)));
    }

    float3 V = normalize(cameraPos - input.pos);
    float NdotV = max(dot(normal, V), 0.f);

    float3 F0 = lerp((.04f).xxx, albedo.rgb, metallic);

    PointLight light = pointlights[0];

    float3 L = normalize(light.position - input.pos);
    float3 H = normalize(L + V);
    float NdotH = max(dot(normal, H), 0.f);
    float NdotL = max(dot(normal, L), 0.f);
    float VdotH = max(dot(V, H), 0.f);

    float3 F = fresnel(VdotH, F0);
    float  D = ggx(pow(roughness, 4), NdotH);
    float  G = smith(pow(roughness, 4), NdotV, NdotL);

    // Cook-Torrance
    float3 nominator = D * G * F;
    float denom = 4.f * NdotV * NdotL;
    float3 specular = nominator / max(denom, .0001f);

    float distance = length(light.position - input.pos);
    float attenuation = 1.f / (distance * distance);
    float3 radiance = light.color * attenuation;

    float3 kS = F;
    float3 kD = 1.f - kS;
    kD *= 1.f - metallic;

    float3 diffuse = (albedo.xyz * kD / 3.14 + specular) * radiance * NdotL;

    return float4(diffuse + albedo.xyz, albedo.w);

    float4 result = 0;

    for (uint i = 0; i < spotLightsNum; ++i)
    {
        result += calculateSpotLight(input, spotlights[i]);
    }

    for (uint i = 0; i < pointLightsNum; ++i)
    {
        result += calculatePointLight(input, pointlights[i]);
    }

    result += calculateDirectionalLight(input.normal, directional);

    result.rgb *= albedo.rgb;
    result.w = albedo.w;
    return result;
}
