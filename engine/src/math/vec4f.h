#ifndef HK_VEC4F_H
#define HK_VEC4F_H

#include "defines.h"
#include "math/utils.h"

#include "vec3f.h"

namespace hkm {

struct vec4f {
    f32 x, y, z, w;

    constexpr vec4f() : x(0), y(0), z(0), w(0) {}
    constexpr vec4f(f32 w) : x(w), y(w), z(w), w(w) {}
    constexpr vec4f(f32 x, f32 y, f32 z, f32 w) : x(x), y(y), z(z), w(w) {}
    constexpr vec4f(const vec3f &v) : x(v.x), y(v.y), z(v.z), w(0) {}
    constexpr vec4f(const vec3f &v, f32 w) : x(v.x), y(v.y), z(v.z), w(w) {}

    constexpr f32& operator [](u32 i) { return ((&x)[i]); }
    constexpr const f32& operator [](u32 i) const { return ((&x)[i]); }

    inline f32 length() const
    {
        return hkm::sqrt(x*x + y*y + z*z + w*w);
    }

    constexpr vec4f operator-() const
    {
        return vec4f(-x, -y, -z, -w);
    }

    constexpr vec4f& operator =(const vec4f &other)
    {
        this->x = other.x;
        this->y = other.y;
        this->z = other.z;
        this->w = other.w;
        return *this;
    }

    constexpr vec4f& operator +=(const vec4f &other)
    {
        x += other.x;
        y += other.y;
        z += other.z;
        w += other.w;
        return *this;
    }

    constexpr vec4f& operator -=(const vec4f &other)
    {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        w -= other.w;
        return *this;
    }

    constexpr vec4f& operator *=(f32 s)
    {
        x *= s;
        y *= s;
        z *= s;
        w *= s;
        return *this;
    }

    constexpr vec4f& operator /=(f32 s)
    {
        s = 1.f / s;
        x *= s;
        y *= s;
        z *= s;
        w *= s;
        return *this;
    }

    constexpr b8 operator ==(const vec4f &other) const
    {
        return (
            this->x == other.x &&
            this->y == other.y &&
            this->z == other.z &&
            this->w == other.w
        );
    }

    constexpr b8 operator !=(const vec4f &other) const
    {
        return !(*this == other);
    }
};

constexpr vec4f operator +(const vec4f &u, const vec4f &v)
{
    return vec4f(u.x + v.x, u.y + v.y, u.z + v.z, u.w + v.w);
}

constexpr vec4f operator -(const vec4f &u, const vec4f &v)
{
    return vec4f(u.x - v.x, u.y - v.y, u.z - v.z, u.w - v.w);
}

constexpr vec4f operator *(const vec4f &u, const vec4f &v)
{
    return vec4f(u.x * v.x, u.y * v.y, u.z * v.z, u.w * v.w);
}

constexpr vec4f operator *(const vec4f &u, f32 s)
{
    return vec4f(u.x * s, u.y * s, u.z * s, u.w * s);
}

constexpr vec4f operator *(f32 s, const vec4f &u)
{
    return u * s;
}

constexpr vec4f operator /(const vec4f &u, f32 s)
{
    s = 1.f / s;
    return vec4f(u.x * s, u.y * s, u.z * s, u.w * s);
}

inline vec4f normalize(const vec4f &v) {
    const f32 length = v.length();
    if (length <= 0) return vec4f(0.f);

    return v / length;
}

constexpr vec4f clamp(const vec4f &v, f32 upper, f32 lower) {
    f32 x = (v.x > upper) ? upper : (v.x < lower) ? lower : v.x;
    f32 y = (v.y > upper) ? upper : (v.y < lower) ? lower : v.y;
    f32 z = (v.z > upper) ? upper : (v.z < lower) ? lower : v.z;
    f32 w = (v.w > upper) ? upper : (v.w < lower) ? lower : v.w;

    return vec4f(x, y, z, w);
}

}
#endif // HK_VEC4F_H
