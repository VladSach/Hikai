#ifndef HK_MAT4F_H
#define HK_MAT4F_H

#include "defines.h"
#include "vec3f.h"
#include "vec4f.h"
#include "mat3f.h"

namespace hkm {

struct mat4f {
    f32 n[4][4];

    constexpr mat4f() : n{0} {};

    mat4f(f32 n00, f32 n01, f32 n02, f32 n03,
          f32 n10, f32 n11, f32 n12, f32 n13,
          f32 n20, f32 n21, f32 n22, f32 n23,
          f32 n30, f32 n31, f32 n32, f32 n33)
    {
        n[0][0] = n00; n[0][1] = n01; n[0][2] = n02; n[0][3] = n03;
        n[1][0] = n10; n[1][1] = n11; n[1][2] = n12; n[1][3] = n13;
        n[2][0] = n20; n[2][1] = n21; n[2][2] = n22; n[2][3] = n23;
        n[3][0] = n30; n[3][1] = n31; n[3][2] = n32; n[3][3] = n33;
    }
    // mat4f(const mat3f &M)
    // {
    //     n[0][0] = M.n[0][0]; n[0][1] = M.n[0][1]; n[0][2] = M.n[0][2];
    //     n[1][0] = M.n[1][0]; n[1][1] = M.n[1][1]; n[1][2] = M.n[1][2];
    //     n[2][0] = M.n[2][0]; n[2][1] = M.n[2][1]; n[2][2] = M.n[2][2];
    //     n[3][0] = 0;         n[3][1] = 0;         n[3][2] = 0;
    //
    //     n[0][3] = 0;
    //     n[1][3] = 0;
    //     n[2][3] = 0;
    //     n[3][3] = 1;
    // }

    constexpr mat4f& operator=(const mat4f &rhs)
    {
        n[0][0] = rhs.n[0][0]; n[0][1] = rhs.n[0][1]; n[0][2] = rhs.n[0][2];
        n[1][0] = rhs.n[1][0]; n[1][1] = rhs.n[1][1]; n[1][2] = rhs.n[1][2];
        n[2][0] = rhs.n[2][0]; n[2][1] = rhs.n[2][1]; n[2][2] = rhs.n[2][2];
        n[3][0] = rhs.n[3][0]; n[3][1] = rhs.n[3][1]; n[3][2] = rhs.n[3][2];

        n[0][3] = rhs.n[0][3];
        n[1][3] = rhs.n[1][3];
        n[2][3] = rhs.n[2][3];
        n[3][3] = rhs.n[3][3];

        return *this;
    }

    constexpr b8 operator ==(const mat4f &rhs) const
    {
        return (
            getRowAsVec4(0) == rhs.getRowAsVec4(0) &&
            getRowAsVec4(1) == rhs.getRowAsVec4(1) &&
            getRowAsVec4(2) == rhs.getRowAsVec4(2) &&
            getRowAsVec4(3) == rhs.getRowAsVec4(3));
    }

    constexpr f32& operator()(u32 i, u32 j) { return n[i][j]; }
    constexpr const f32& operator()(u32 i, u32 j) const { return n[i][j]; }

    static mat4f identity()
    {
        return mat4f(1, 0, 0, 0,
                     0, 1, 0, 0,
                     0, 0, 1, 0,
                     0, 0, 0, 1);
    }

    constexpr vec3f getRowAsVec3(u32 row) const
    {
        return vec3f(n[row][0], n[row][1], n[row][2]);
    }

    constexpr vec4f getRowAsVec4(u32 row) const
    {
        return vec4f(n[row][0], n[row][1], n[row][2], n[row][3]);
    }

    vec3f& getRowAsVec3(u32 row)
    {
        f32 *temp[3] = {0};
        for (u32 i = 0; i < 3; i++) {
            temp[i] = &n[row][i];
        }

        return *reinterpret_cast<vec3f*>(*temp);
    }

    void asMat3(const mat3f &m)
    {
        n[0][0] = m(0, 0); n[1][0] = m(1, 0); n[2][0] = m(2, 0);
        n[0][1] = m(0, 1); n[1][1] = m(1, 1); n[2][1] = m(2, 1);
        n[0][2] = m(0, 2); n[1][2] = m(1, 2); n[2][2] = m(2, 2);
    }

    // mat3f mat3f()
    // {
    //     return mat3f(n[0][0], n[0][1], n[0][2],
    //                  n[1][0], n[1][1], n[1][2],
    //                  n[2][0], n[2][1], n[2][2]);
    // }
};

constexpr vec4f operator *(const mat4f &M, const vec4f &v)
{
    return vec4f(M(0, 0) * v.x + M(1, 0) * v.y + M(2, 0) * v.z + M(3, 0) * v.w,
                 M(0, 1) * v.x + M(1, 1) * v.y + M(2, 1) * v.z + M(3, 1) * v.w,
                 M(0, 2) * v.x + M(1, 2) * v.y + M(2, 2) * v.z + M(3, 2) * v.w,
                 M(0, 3) * v.x + M(1, 3) * v.y + M(2, 3) * v.z + M(3, 3) * v.w);
}

inline mat4f operator *(const mat4f &A, const mat4f &B)
{
    return mat4f(
        A(0, 0) * B(0, 0) + A(0, 1) * B(1, 0) + A(0, 2) * B(2, 0) + A(0, 3) * B(3, 0),
        A(0, 0) * B(0, 1) + A(0, 1) * B(1, 1) + A(0, 2) * B(2, 1) + A(0, 3) * B(3, 1),
        A(0, 0) * B(0, 2) + A(0, 1) * B(1, 2) + A(0, 2) * B(2, 2) + A(0, 3) * B(3, 2),
        A(0, 0) * B(0, 3) + A(0, 1) * B(1, 3) + A(0, 2) * B(2, 3) + A(0, 3) * B(3, 3),
        A(1, 0) * B(0, 0) + A(1, 1) * B(1, 0) + A(1, 2) * B(2, 0) + A(1, 3) * B(3, 0),
        A(1, 0) * B(0, 1) + A(1, 1) * B(1, 1) + A(1, 2) * B(2, 1) + A(1, 3) * B(3, 1),
        A(1, 0) * B(0, 2) + A(1, 1) * B(1, 2) + A(1, 2) * B(2, 2) + A(1, 3) * B(3, 2),
        A(1, 0) * B(0, 3) + A(1, 1) * B(1, 3) + A(1, 2) * B(2, 3) + A(1, 3) * B(3, 3),
        A(2, 0) * B(0, 0) + A(2, 1) * B(1, 0) + A(2, 2) * B(2, 0) + A(2, 3) * B(3, 0),
        A(2, 0) * B(0, 1) + A(2, 1) * B(1, 1) + A(2, 2) * B(2, 1) + A(2, 3) * B(3, 1),
        A(2, 0) * B(0, 2) + A(2, 1) * B(1, 2) + A(2, 2) * B(2, 2) + A(2, 3) * B(3, 2),
        A(2, 0) * B(0, 3) + A(2, 1) * B(1, 3) + A(2, 2) * B(2, 3) + A(2, 3) * B(3, 3),
        A(3, 0) * B(0, 0) + A(3, 1) * B(1, 0) + A(3, 2) * B(2, 0) + A(3, 3) * B(3, 0),
        A(3, 0) * B(0, 1) + A(3, 1) * B(1, 1) + A(3, 2) * B(2, 1) + A(3, 3) * B(3, 1),
        A(3, 0) * B(0, 2) + A(3, 1) * B(1, 2) + A(3, 2) * B(2, 2) + A(3, 3) * B(3, 2),
        A(3, 0) * B(0, 3) + A(3, 1) * B(1, 3) + A(3, 2) * B(2, 3) + A(3, 3) * B(3, 3)
    );
}

// inline vec3f transformVec(const mat4f &M, const vec3f &v)
// {
//     vec4f homogeneous = { v.x, v.y, v.z, 0 };
//     vec4f transformed = M * homogeneous;
//     return vec3f( transformed.x,
//                   transformed.y,
//                   transformed.z );
// }

inline vec3f transformPoint(const mat4f &M, const vec3f &v)
{
    vec4f homogeneous = { v.x, v.y, v.z, 1 };
    vec4f transformed = M * homogeneous;

    transformed.w = 1.f / transformed.w;
    return vec3f( transformed.x * transformed.w,
                  transformed.y * transformed.w,
                  transformed.z * transformed.w );
}

inline mat4f transpose(const mat4f &m)
{
    return mat4f(m(0, 0), m(1, 0), m(2, 0), m(3, 0),
                 m(0, 1), m(1, 1), m(2, 1), m(3, 1),
                 m(0, 2), m(1, 2), m(2, 2), m(3, 2),
                 m(0, 3), m(1, 3), m(2, 3), m(3, 3));
}

inline mat4f inverse(const mat4f &m)
{
    f32 A2323 = m(2, 2) * m(3, 3) - m(2, 3) * m(3, 2);
    f32 A1323 = m(2, 1) * m(3, 3) - m(2, 3) * m(3, 1);
    f32 A1223 = m(2, 1) * m(3, 2) - m(2, 2) * m(3, 1);
    f32 A0323 = m(2, 0) * m(3, 3) - m(2, 3) * m(3, 0);
    f32 A0223 = m(2, 0) * m(3, 2) - m(2, 2) * m(3, 0);
    f32 A0123 = m(2, 0) * m(3, 1) - m(2, 1) * m(3, 0);
    f32 A2313 = m(1, 2) * m(3, 3) - m(1, 3) * m(3, 2);
    f32 A1313 = m(1, 1) * m(3, 3) - m(1, 3) * m(3, 1);
    f32 A1213 = m(1, 1) * m(3, 2) - m(1, 2) * m(3, 1);
    f32 A2312 = m(1, 2) * m(2, 3) - m(1, 3) * m(2, 2);
    f32 A1312 = m(1, 1) * m(2, 3) - m(1, 3) * m(2, 1);
    f32 A1212 = m(1, 1) * m(2, 2) - m(1, 2) * m(2, 1);
    f32 A0313 = m(1, 0) * m(3, 3) - m(1, 3) * m(3, 0);
    f32 A0213 = m(1, 0) * m(3, 2) - m(1, 2) * m(3, 0);
    f32 A0312 = m(1, 0) * m(2, 3) - m(1, 3) * m(2, 0);
    f32 A0212 = m(1, 0) * m(2, 2) - m(1, 2) * m(2, 0);
    f32 A0113 = m(1, 0) * m(3, 1) - m(1, 1) * m(3, 0);
    f32 A0112 = m(1, 0) * m(2, 1) - m(1, 1) * m(2, 0);

    f32 det;
    det = m(0, 0) * (m(1, 1) * A2323 - m(1, 2) * A1323 + m(1, 3) * A1223)
        - m(0, 1) * (m(1, 0) * A2323 - m(1, 2) * A0323 + m(1, 3) * A0223)
        + m(0, 2) * (m(1, 0) * A1323 - m(1, 1) * A0323 + m(1, 3) * A0123)
        - m(0, 3) * (m(1, 0) * A1223 - m(1, 1) * A0223 + m(1, 2) * A0123);

    det = 1 / det;

    mat4f im;
    im(0, 0) = det *   (m(1, 1) * A2323 - m(1, 2) * A1323 + m(1, 3) * A1223);
    im(0, 1) = det * - (m(0, 1) * A2323 - m(0, 2) * A1323 + m(0, 3) * A1223);
    im(0, 2) = det *   (m(0, 1) * A2313 - m(0, 2) * A1313 + m(0, 3) * A1213);
    im(0, 3) = det * - (m(0, 1) * A2312 - m(0, 2) * A1312 + m(0, 3) * A1212);
    im(1, 0) = det * - (m(1, 0) * A2323 - m(1, 2) * A0323 + m(1, 3) * A0223);
    im(1, 1) = det *   (m(0, 0) * A2323 - m(0, 2) * A0323 + m(0, 3) * A0223);
    im(1, 2) = det * - (m(0, 0) * A2313 - m(0, 2) * A0313 + m(0, 3) * A0213);
    im(1, 3) = det *   (m(0, 0) * A2312 - m(0, 2) * A0312 + m(0, 3) * A0212);
    im(2, 0) = det *   (m(1, 0) * A1323 - m(1, 1) * A0323 + m(1, 3) * A0123);
    im(2, 1) = det * - (m(0, 0) * A1323 - m(0, 1) * A0323 + m(0, 3) * A0123);
    im(2, 2) = det *   (m(0, 0) * A1313 - m(0, 1) * A0313 + m(0, 3) * A0113);
    im(2, 3) = det * - (m(0, 0) * A1312 - m(0, 1) * A0312 + m(0, 3) * A0112);
    im(3, 0) = det * - (m(1, 0) * A1223 - m(1, 1) * A0223 + m(1, 2) * A0123);
    im(3, 1) = det *   (m(0, 0) * A1223 - m(0, 1) * A0223 + m(0, 2) * A0123);
    im(3, 2) = det * - (m(0, 0) * A1213 - m(0, 1) * A0213 + m(0, 2) * A0113);
    im(3, 3) = det *   (m(0, 0) * A1212 - m(0, 1) * A0212 + m(0, 2) * A0112);

    return im;
}

}

#endif // HK_MAT4F_H
