#ifndef HK_TRANSFORM_H
#define HK_TRANSFORM_H

#include "math/hkmath.h"

struct Transform {
    hkm::vec3f scale;
    hkm::vec3f pos;
    hkm::quaternion rotation;

    Transform() : rotation(), pos(), scale() {}

    Transform(const hkm::vec3f &position,
              const hkm::vec3f &scale,
              const hkm::quaternion &rotation = hkm::quaternion::identity())
        : rotation(rotation), pos(position), scale(scale)
        {}

    Transform(hkm::mat4f M)
    {
        pos = M.getRowAsVec3(3);

        scale = hkm::vec3f(
            hkm::vec3f(M(0, 0), M(0, 1), M(0, 2)).length(),
            hkm::vec3f(M(1, 0), M(1, 1), M(1, 2)).length(),
            hkm::vec3f(M(2, 0), M(2, 1), M(2, 2)).length()
        );

        float sx = scale.x;
        float sy = scale.y;
        float sz = scale.z;
        hkm::mat4f rot = {
            M(0, 0) / sx, M(0, 1) / sy, M(0, 2) / sz, 0,
            M(1, 0) / sx, M(1, 1) / sy, M(1, 2) / sz, 0,
            M(2, 0) / sx, M(2, 1) / sy, M(2, 2) / sz, 0,
            0, 0, 0, 1
        };

        rotation.setRotationMatrix4(rot);
    }

    hkm::mat4f toMat4f() const
    {
        hkm::mat4f translateMatrix = hkm::mat4f::identity();
        hkm::mat4f scaleMatrix = hkm::mat4f::identity();
        hkm::mat4f rotationMatrix = rotation.rotmat4f();

        // transposed
        translateMatrix(3, 0) = pos.x;
        translateMatrix(3, 1) = pos.y;
        translateMatrix(3, 2) = pos.z;

        scaleMatrix(0, 0) = scale.x;
        scaleMatrix(1, 1) = scale.y;
        scaleMatrix(2, 2) = scale.z;

        return scaleMatrix * rotationMatrix * translateMatrix;
    }

    hkm::mat4f inverseMat4() const
    {
        hkm::mat4f translateMatrix = hkm::mat4f::identity();
        hkm::mat4f scaleMatrix = hkm::mat4f::identity();
        hkm::mat4f rotationMatrix = rotation.rotmat4f();

        translateMatrix(3, 0) = -pos.x;
        translateMatrix(3, 1) = -pos.y;
        translateMatrix(3, 2) = -pos.z;

        scaleMatrix(0, 0) = 1 / scale.x;
        scaleMatrix(1, 1) = 1 / scale.y;
        scaleMatrix(2, 2) = 1 / scale.z;

        return scaleMatrix * rotationMatrix * translateMatrix;
    }
};

#endif // HK_TRANSFORM_H
