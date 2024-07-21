#ifndef HK_VEC2F_H
#define HK_VEC2F_H

#include "defines.h"

namespace hkm {

struct vec2f {
    f32 x, y;

    constexpr vec2f() : x(0), y(0) {}
    constexpr vec2f(f32 w) : x(w), y(w) {}
    constexpr vec2f(f32 x, f32 y) : x(x), y(y) {}

    constexpr f32& operator [](u32 i) { return ((&x)[i]); }
    constexpr const f32& operator [](u32 i) const { return ((&x)[i]); }

    inline f32 length() const
    {
        return std::sqrt(x*x + y*y);
    }

    constexpr vec2f operator-() const
    {
        return vec2f(-x, -y);
    }

    constexpr vec2f& operator =(const vec2f &other)
    {
        this->x = other.x;
        this->y = other.y;
        return *this;
    }

    constexpr vec2f& operator +=(const vec2f &other)
    {
        x += other.x;
        y += other.y;
        return *this;
    }

    constexpr vec2f& operator -=(const vec2f &other)
    {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    constexpr vec2f& operator *=(f32 s)
    {
        x *= s;
        y *= s;
        return *this;
    }

    constexpr vec2f& operator /=(f32 s)
    {
        s = 1.f / s;
        x *= s;
        y *= s;
        return *this;
    }

    constexpr b8 operator ==(const vec2f &other) const
    {
        return this->x == other.x && this->y == other.y;
    }

    constexpr b8 operator !=(const vec2f &other) const
    {
        return !(*this == other);
    }
};

constexpr vec2f operator +(const vec2f &u, const vec2f &v)
{
    return vec2f(u.x + v.x, u.y + v.y);
}

constexpr vec2f operator -(const vec2f &u, const vec2f &v)
{
    return vec2f(u.x - v.x, u.y - v.y);
}

constexpr vec2f operator *(const vec2f &u, const vec2f &v)
{
    return vec2f(u.x * v.x, u.y * v.y);
}

constexpr vec2f operator *(const vec2f &u, f32 s)
{
    return vec2f(u.x * s, u.y * s);
}

constexpr vec2f operator *(f32 s, const vec2f &u)
{
    return u * s;
}

constexpr vec2f operator /(const vec2f &u, f32 s)
{
    s = 1.f / s;
    return vec2f(u.x * s, u.y * s);
}

inline vec2f normalize(const vec2f &v) {
    const f32 length = v.length();
    if (length <= 0) return vec2f(0.f);

    return v / length;
}

constexpr f32 dot(const vec2f &u, const vec2f &v)
{
    return u.x * v.x + u.y * v.y;
}

constexpr vec2f clamp(const vec2f &v, f32 upper, f32 lower) {
    f32 x = (v.x > upper) ? upper : (v.x < lower) ? lower : v.x;
    f32 y = (v.y > upper) ? upper : (v.y < lower) ? lower : v.y;

    return vec2f(x, y);
}

}
#endif // HK_VEC2F_H
