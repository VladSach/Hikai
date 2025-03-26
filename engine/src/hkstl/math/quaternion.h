#ifndef HK_QUATERNION_H
#define HK_QUATERNION_H

#include "utility/hktypes.h"

#include "constants.h"
#include "vec3f.h"
#include "mat3f.h"
#include "mat4f.h"

namespace hkm {

struct quaternion {
    f32 x, y, z, w;

    constexpr quaternion() : x(0), y(0), z(0), w(1) {}
    constexpr quaternion(f32 a, f32 b, f32 c, f32 s) : x(a), y(b), z(c), w(s) {}
    constexpr quaternion(vec3f v, f32 s) : x(v.x), y(v.y), z(v.z), w(s) {}

    constexpr f32& operator [](u32 i) { return ((&x)[i]); }
    constexpr const f32& operator [](u32 i) const { return ((&x)[i]); }

    inline f32 length() const { return hkm::sqrt(x*x + y*y + z*z + w*w); }

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

    constexpr mat4f rotmat4f() const
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

        return mat4f(
            1.f - 2.f * (y2 + z2), 2.f * (xy - wz),       2.f * (xz + wy),   0,
            2.f * (xy + wz),       1.f - 2.f * (x2 + z2), 2.f * (yz - wx),   0,
            2.f * (xz - wy),       2.f * (yz + wx),   1.f - 2.f * (x2 + y2), 0,
            0, 0, 0, 1
        );
    }

    void setRotationMatrix4(const mat4f &m)
    {
        float m00 = m(0, 0);
        float m11 = m(1, 1);
        float m22 = m(2, 2);
        float sum = m00 + m11 + m22;

        if (sum > .0f) {
            w = hkm::sqrt(sum + 1.f) * .5f;
            float f = .25f / w;

            x = (m(2, 1) - m(1, 2)) * f;
            y = (m(0, 2) - m(2, 0)) * f;
            z = (m(1, 0) - m(0, 1)) * f;
        } else if ((m00 > m11) && (m00 > m22)) {
            x = hkm::sqrt(m00 - m11 - m22 + 1.f) * .5f;
            float f = .25f / x;

            y = (m(1, 0) + m(0, 1)) * f;
            z = (m(0, 2) + m(2, 0)) * f;
            w = (m(2, 1) - m(1, 2)) * f;
        } else if (m11 > m22) {
            y = hkm::sqrt(m11 - m00 - m22 + 1.f) * .5f;
            float f = .25f / y;

            x = (m(1, 0) + m(0, 1)) * f;
            z = (m(2, 1) + m(1, 2)) * f;
            w = (m(0, 2) - m(2, 0)) * f;
        } else {
            z = hkm::sqrt(m22 - m00 - m11 + 1.f) * .5f;
            float f = .25f / z;

            x = (m(0, 2) + m(2, 0)) * f;
            y = (m(2, 1) + m(1, 2)) * f;
            w = (m(1, 0) - m(0, 1)) * f;
        }
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

// Angle passed in radians
inline quaternion fromAxisAngle(const vec3f &v, f32 angle)
{
    vec3f N = normalize(v);

    f32 half_angle = angle * .5f;
    f32 sin_angle = std::sin(half_angle);
    f32 cos_angle = std::cos(half_angle);

    f32 x = N.x * sin_angle;
    f32 y = N.y * sin_angle;
    f32 z = N.z * sin_angle;

    return normalize(quaternion(x, y, z, cos_angle));
}

// Angles passed in radians
inline quaternion fromEulerAngles(const vec3f &angles)
{
    f32 yaw   = angles.x;
    f32 pitch = angles.y;
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

// Angles returned in radians
inline vec3f toEulerAngles(const quaternion &quat)
{
    f32 roll = 0;
    f32 pitch = 0;
    f32 yaw = 0;

    f32 x = quat.x;
    f32 y = quat.y;
    f32 z = quat.z;
    f32 w = quat.w;

    // https://www.euclideanspace.com/maths/geometry/rotations/conversions/quaternionToEuler/
    f32 test = x * y - z * w;
    f32 unit = x*x + y*y + z*z + w*w;
    if (test > .499f * unit) {
        yaw = 2.f * std::atan2(x, w);
        roll = pi * .5f;
        pitch = 0.f;
        return vec3f(pitch, yaw, roll);
    }

    if (test < -.499f * unit) {
        yaw = 2.f * std::atan2(x, w);
        roll = pi * .5f;
        pitch = 0.f;
        return vec3f(-pitch, -yaw, roll);
    }

    // Pitch (x-axis rotation)
    f32 sinr_cosp = 2.f * (w * x + y * z);
    f32 cosr_cosp = 1.f - 2.f * (x * x + y * y);
    pitch = std::atan2(sinr_cosp, cosr_cosp);

    // Yaw (y-axis rotation)
    f32 sinp = std::sqrt(1.f + 2.f * (w * y - z * x));
    f32 cosp = std::sqrt(1.f - 2.f * (w * y - z * x));
    yaw = 2.f * std::atan2(sinp, cosp) - pi * .5f;

    // Roll (z-axis rotation)
    f32 siny_cosp = 2.f * (w * z + x * y);
    f32 cosy_cosp = 1.f - 2.f * (y * y + z * z);
    roll = std::atan2(siny_cosp, cosy_cosp);

    return vec3f(pitch, yaw, roll);
}

}

#endif // HK_QUATERNION_H
