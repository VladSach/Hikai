#ifndef HK_LIGHT_H
#define HK_LIGHT_H

#include "math/hkmath.h"

namespace hk {

// FIX: delete
struct Light {
    hkm::vec4f color;
    f32 intensity;

    constexpr Light() : color(), intensity(0.f) {}
    constexpr Light(const hkm::vec4f &color, f32 intensity)
    : color(color), intensity(intensity)
    {}

};

class PointLight {
public:
    hkm::vec4f color;
    hkm::vec3f position; // FIX: temp
    f32 intensity;

public:
    PointLight()
    : position(), color(), intensity(0.f)
    {}

    PointLight(const hkm::vec3f &pos, const hkm::vec4f &color)
    : position(pos), color(color)
    {}

};

class SpotLight {
public:
    hkm::vec4f position;
    hkm::vec4f direction;

    hkm::vec3f color;

    float innerCutoff;
    float outerCutoff;

public:
    SpotLight()
    : position(0), direction(0), color(0),
      innerCutoff(0), outerCutoff(0)
    {}

    SpotLight(const hkm::vec3f &pos, const hkm::vec3f &dir,
              const hkm::vec3f &color,
              float in, float out)
    : position(pos), direction(dir), color(color),
      innerCutoff(in), outerCutoff(out)
    {}

};

}

#endif // HK_LIGHT_H
