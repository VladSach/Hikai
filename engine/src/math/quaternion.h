#ifndef HK_QUATERNION_H
#define HK_QUATERNION_H

#include "defines.h"
#include "vec3f.h"
#include "mat3f.h"

namespace hkm {

struct quaternion {
    f32 x, y, z, w;

    constexpr quaternion() : x(0), y(0), z(0), w(1) {}
    constexpr quaternion(f32 a, f32 b, f32 c, f32 s) : x(a), y(b), z(c), w(s) {}
    constexpr quaternion(vec3f v, f32 s) : x(v.x), y(v.y), z(v.z), w(s) {}

    constexpr f32& operator [](u32 i) { return ((&x)[i]); }
    constexpr const f32& operator [](u32 i) const { return ((&x)[i]); }

    inline f32 length() const { return std::sqrt(x*x + y*y + z*z + w*w); }

    constexpr static quaternion identity()
    {
        return quaternion(0, 0, 0, 1);
    }

    constexpr const vec3f vec3() const
    {
        return vec3f(x, y, z);
    }

    constexpr b8 operator ==(const quaternion &rhs) const
    {
        return (
            this->x == rhs.x &&
            this->y == rhs.y &&
            this->z == rhs.z &&
            this->w == rhs.w
        );
    }

    constexpr quaternion& operator *=(f32 s)
    {
        x *= s;
        y *= s;
        z *= s;
        w *= s;
        return *this;
    }

    constexpr quaternion& operator *=(const quaternion &rhs)
    {
        x = rhs.w * x + rhs.x * w + rhs.y * z - rhs.z * y;
        y = rhs.w * y - rhs.x * z + rhs.y * w + rhs.z * x;
        z = rhs.w * z + rhs.x * y - rhs.y * x + rhs.z * w;
        w = rhs.w * w - rhs.x * x - rhs.y * y - rhs.z * z;
        return *this;
    }

    constexpr quaternion& operator /=(f32 s)
    {
        s = 1.f / s;
        x *= s;
        y *= s;
        z *= s;
        w *= s;
        return *this;
    }

    constexpr mat3f rotmat() const
    {
        f32 x2 = x * x;
        f32 y2 = y * y;
        f32 z2 = z * z;
        f32 xy = x * y;
        f32 xz = x * z;
        f32 yz = y * z;
        f32 wx = w * x;
        f32 wy = w * y;
        f32 wz = w * z;

        return mat3f(
            1.f - 2.f * (y2 + z2), 2.f * (xy - wz),       2.f * (xz + wy),
            2.f * (xy + wz),       1.f - 2.f * (x2 + z2), 2.f * (yz - wx),
            2.f * (xz - wy),       2.f * (yz + wx),       1.f - 2.f * (x2 + y2)
        );
    }
};

constexpr quaternion operator *(const quaternion &q1, const quaternion &q2)
{
    return quaternion(
        q2.w * q1.x + q2.x * q1.w + q2.y * q1.z - q2.z * q1.y,
        q2.w * q1.y - q2.x * q1.z + q2.y * q1.w + q2.z * q1.x,
        q2.w * q1.z + q2.x * q1.y - q2.y * q1.x + q2.z * q1.w,
        q2.w * q1.w - q2.x * q1.x - q2.y * q1.y - q2.z * q1.z);
}

constexpr quaternion operator /(const quaternion &q, f32 s)
{
    s = 1.f / s;
    return quaternion(q.x * s, q.y * s, q.z * s, q.w * s);
}

constexpr quaternion normalize(const quaternion &q)
{
    // if (q.w < 0) {
    //     return q / -q.length();
    // }
    return q / q.length();
}

constexpr vec3f operator *(const vec3f &v, const quaternion &q)
{
    const vec3f& b = q.vec3();
    f32 b2 = b.x * b.x + b.y * b.y + b.z * b.z;
    return (
        v * (q.w * q.w - b2) +
        b * (dot(v, b) * 2.f) +
        cross(b, v) * (q.w * 2.f)
    );
}

constexpr vec3f operator *(const quaternion &q, const vec3f &v)
{
    return v * q;
}

// angle passed in radians
inline quaternion fromAxisAngle(const vec3f &v, f32 angle) {
    vec3f N = normalize(v);

    f32 halfAngle = angle * .5f;
    f32 sinAngle = sin(halfAngle);
    f32 cosAngle = cos(halfAngle);

    f32 x = N.x * sinAngle;
    f32 y = N.y * sinAngle;
    f32 z = N.z * sinAngle;

    return normalize(quaternion(x, y, z, cosAngle));
}

inline quaternion fromEulerAngles(const vec3f &angles) {
    f32 pitch = angles.x;
    f32 yaw   = angles.y;
    f32 roll  = angles.z;

    f32 cy = std::cos(yaw   * .5f);
    f32 sy = std::sin(yaw   * .5f);
    f32 cp = std::cos(pitch * .5f);
    f32 sp = std::sin(pitch * .5f);
    f32 cr = std::cos(roll  * .5f);
    f32 sr = std::sin(roll  * .5f);

    f32 w = cr * cp * cy + sr * sp * sy;
    f32 y = cr * cp * sy - sr * sp * cy;
    f32 x = cr * sp * cy + sr * cp * sy;
    f32 z = sr * cp * cy - cr * sp * sy;
    return quaternion(x, y, z, w);
}

}

#endif // HK_QUATERNION_H
