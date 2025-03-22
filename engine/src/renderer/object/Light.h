#ifndef HK_LIGHT_H
#define HK_LIGHT_H

#include "math/hkmath.h"

namespace hk {

struct Light {
    enum class Type {
        POINT_LIGHT,
        SPOT_LIGHT,
        DIRECTIONAL_LIGHT
    } type;

    // FIX: change to vec3f
    hkm::vec4f color;
    f32 intensity; // intensity can be color.w

    f32 range;

    // Spotlight only
    // hkm::vec3f dir;
    //
    // change to f32 penumbra? (also removes need to check if outer < inner)
    f32 inner_cutoff;
    f32 outer_cutoff;

    // constexpr Light() : color(), intensity(.0f) {}
    // constexpr Light(const hkm::vec4f &color, f32 intensity)
    // : color(color), intensity(intensity)
    // {}
    //
};

struct SpotLight {
    hkm::vec3f dir;
    hkm::vec4f color;

    f32 inner_cutoff;
    f32 outer_cutoff;

    constexpr SpotLight()
    : dir(), color(), inner_cutoff(.0f), outer_cutoff(.0f)
    {}

    constexpr SpotLight(
        const hkm::vec3f &direction,
        const hkm::vec4f &color,
        f32 inner, f32 outer)
    : dir(direction), color(color), inner_cutoff(inner), outer_cutoff(outer)
    {}
};

}

#endif // HK_LIGHT_H
