#ifndef HK_VEC3F_H
#define HK_VEC3F_H

#include "defines.h"

namespace hkm {

struct vec3f {
    f32 x, y, z;

    constexpr vec3f() : x(0), y(0), z(0) {}
    constexpr vec3f(f32 w) : x(w), y(w), z(w) {}
    constexpr vec3f(f32 x, f32 y, f32 z) : x(x), y(y), z(z) {}

    constexpr f32& operator [](u32 i) { return ((&x)[i]); }
    constexpr const f32& operator [](u32 i) const { return ((&x)[i]); }

    inline f32 length() const
    {
        return std::sqrt(x*x + y*y + z*z);
    }

    constexpr vec3f operator-() const
    {
        return vec3f(-x, -y, -z);
    }

    constexpr vec3f& operator =(const vec3f &other)
    {
        this->x = other.x;
        this->y = other.y;
        this->z = other.z;
        return *this;
    }

    constexpr vec3f& operator +=(const vec3f &other)
    {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }

    constexpr vec3f& operator -=(const vec3f &other)
    {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        return *this;
    }

    constexpr vec3f& operator *=(f32 s)
    {
        x *= s;
        y *= s;
        z *= s;
        return *this;
    }

    constexpr vec3f& operator /=(f32 s)
    {
        s = 1.f / s;
        x *= s;
        y *= s;
        z *= s;
        return *this;
    }

    constexpr b8 operator ==(const vec3f &other) const
    {
        return (
            this->x == other.x &&
            this->y == other.y &&
            this->z == other.z
        );
    }

    constexpr b8 operator !=(const vec3f &other) const
    {
        return !(*this == other);
    }
};

constexpr vec3f operator +(const vec3f &u, const vec3f &v)
{
    return vec3f(u.x + v.x, u.y + v.y, u.z + v.z);
}

constexpr vec3f operator -(const vec3f &u, const vec3f &v)
{
    return vec3f(u.x - v.x, u.y - v.y, u.z - v.z);
}

constexpr vec3f operator *(const vec3f &u, const vec3f &v)
{
    return vec3f(u.x * v.x, u.y * v.y, u.z * v.z);
}

constexpr vec3f operator *(const vec3f &u, f32 s)
{
    return vec3f(u.x * s, u.y * s, u.z * s);
}

constexpr vec3f operator *(f32 s, const vec3f &u)
{
    return u * s;
}

constexpr vec3f operator /(const vec3f &u, f32 s)
{
    s = 1.f / s;
    return vec3f(u.x * s, u.y * s, u.z * s);
}

inline vec3f normalize(const vec3f &v) {
    const f32 length = v.length();
    if (length <= 0) return vec3f(0.f);

    return v / length;
}

constexpr f32 dot(const vec3f &u, const vec3f &v)
{
    return u.x * v.x + u.y * v.y + u.z * v.z;
}

constexpr vec3f cross(const vec3f &u, const vec3f &v)
{
    return vec3f(u.y * v.z - u.z * v.y,
                 u.z * v.x - u.x * v.z,
                 u.x * v.y - u.y * v.x);
}

constexpr vec3f clamp(const vec3f &v, f32 upper, f32 lower) {
    f32 x = (v.x > upper) ? upper : (v.x < lower) ? lower : v.x;
    f32 y = (v.y > upper) ? upper : (v.y < lower) ? lower : v.y;
    f32 z = (v.z > upper) ? upper : (v.z < lower) ? lower : v.z;

    return vec3f(x, y, z);
}

}
#endif // HK_VEC3F_H
