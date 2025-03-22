#ifndef HK_PBR_HLSLI
#define HK_PBR_HLSLI

#include "globals.hlsli"

// Schlick's approximation of Fresnel reflectance
float3 fresnel(float VdotH, float3 F0)
{
    return F0 + (1.f - F0) * pow(max(1.f - VdotH, .0f), 5.f);
}

// Height-correlated Smith G2 for GGX,
// Filament, 4.4.2 Geometric shadowing
// rough4 is initial roughness value in power of 4
float smith(float rough4, float NoV, float NoL)
{
    NoV *= NoV;
    NoL *= NoL;
    return 2.0 / ( sqrt(1 + rough4 * (1 - NoV) / NoV) + sqrt(1 + rough4 * (1 - NoL) / NoL) );
}

/* ===== Isotropic Normal Distribution Functions ===== */

// GGX/Trowbridge-Reitz
// Real-Time Rendering 4th Edition, page 340, equation 9.41
// rough4 is initial roughness value in power of 4
float GGX(float rough4, float NoH)
{
    float denom = NoH * NoH * (rough4 - 1.f) + 1.f;
    denom = PI * denom * denom;
    return rough4 / denom;
}

#endif // HK_PBR_HLSLI
