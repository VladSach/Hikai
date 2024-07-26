#ifndef HK_MAT3F_H
#define HK_MAT3F_H

#include "defines.h"
#include "vec3f.h"
#include "vec4f.h"

namespace hkm {

struct mat3f {
    f32 n[3][3];

    constexpr mat3f() : n{0} {};

    constexpr mat3f(f32 n00, f32 n01, f32 n02,
                    f32 n10, f32 n11, f32 n12,
                    f32 n20, f32 n21, f32 n22) :
        n{
            {n00, n01, n02},
            {n10, n11, n12},
            {n20, n21, n22}
        }
    {}

    constexpr mat3f& operator =(const mat3f &rhs)
    {
        n[0][0] = rhs.n[0][0]; n[0][1] = rhs.n[0][1]; n[0][2] = rhs.n[0][2];
        n[1][0] = rhs.n[1][0]; n[1][1] = rhs.n[1][1]; n[1][2] = rhs.n[1][2];
        n[2][0] = rhs.n[2][0]; n[2][1] = rhs.n[2][1]; n[2][2] = rhs.n[2][2];
        return *this;
    }

    constexpr b8 operator ==(const mat3f &rhs) const
    {
        return (
            getRowAsVec3(0) == rhs.getRowAsVec3(0) &&
            getRowAsVec3(1) == rhs.getRowAsVec3(1) &&
            getRowAsVec3(2) == rhs.getRowAsVec3(2));
    }

    constexpr f32& operator()(u32 i, u32 j) { return n[i][j]; }
    constexpr const f32& operator()(u32 i, u32 j) const { return n[i][j]; }

    constexpr static mat3f identity()
    {
        return mat3f(1, 0, 0,
                     0, 1, 0,
                     0, 0, 1);
    }

    constexpr vec3f getRowAsVec3(u32 row) const
    {
        return vec3f(n[row][0], n[row][1], n[row][2]);
    }

    vec3f& getRowAsVec3(u32 row)
    {
        f32 *temp[3] = {0};
        for (u32 i = 0; i < 3; i++) {
            temp[i] = &n[row][i];
        }

        return *reinterpret_cast<vec3f*>(*temp);
    }
};

constexpr vec3f operator *(const mat3f &M, const vec3f &v)
{
    return vec3f(M(0, 0) * v.x + M(1, 0) * v.y + M(2, 0) * v.z,
                 M(0, 1) * v.x + M(1, 1) * v.y + M(2, 1) * v.z,
                 M(0, 2) * v.x + M(1, 2) * v.y + M(2, 2) * v.z);
}

inline mat3f operator *(const mat3f &A, const mat3f &B)
{
    return mat3f
    (
        A(0, 0) * B(0, 0) + A(0, 1) * B(1, 0) + A(0, 2) * B(2, 0),
        A(0, 0) * B(0, 1) + A(0, 1) * B(1, 1) + A(0, 2) * B(2, 1),
        A(0, 0) * B(0, 2) + A(0, 1) * B(1, 2) + A(0, 2) * B(2, 2),
        A(1, 0) * B(0, 0) + A(1, 1) * B(1, 0) + A(1, 2) * B(2, 0),
        A(1, 0) * B(0, 1) + A(1, 1) * B(1, 1) + A(1, 2) * B(2, 1),
        A(1, 0) * B(0, 2) + A(1, 1) * B(1, 2) + A(1, 2) * B(2, 2),
        A(2, 0) * B(0, 0) + A(2, 1) * B(1, 0) + A(2, 2) * B(2, 0),
        A(2, 0) * B(0, 1) + A(2, 1) * B(1, 1) + A(2, 2) * B(2, 1),
        A(2, 0) * B(0, 2) + A(2, 1) * B(1, 2) + A(2, 2) * B(2, 2)
    );
}

inline mat3f transpose(const mat3f &m)
{
    return mat3f(m(0, 0), m(1, 0), m(2, 0),
                 m(0, 1), m(1, 1), m(2, 1),
                 m(0, 2), m(1, 2), m(2, 2));
}

}

#endif // HK_MAT3F_H
