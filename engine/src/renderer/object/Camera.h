#ifndef HK_CAMERA_H
#define HK_CAMERA_H

#include "hkcommon.h"

#include "hkstl/math/hkmath.h"

class Camera {
public:
    HKAPI void setPerspective(f32 fov, f32 aspect, f32 nearPlane, f32 farPlane);

    HKAPI void lookAt(const hkm::vec3f &direction);
    HKAPI void fixedBottomRotation(const hkm::vec2f &angles);

    HKAPI void setWorldOffset(const hkm::vec3f &offset);
    HKAPI void addWorldOffset(const hkm::vec3f &offset);
    HKAPI void addRelativeOffset(const hkm::vec3f &offset);

    HKAPI void setWorldAngles(const hkm::vec3f &angles);
    HKAPI void addWorldAngles(const hkm::vec3f &angles);
    HKAPI void addRelativeAngles(const hkm::vec3f &angles);

    HKAPI void update();

public:
    HKAPI constexpr hkm::vec3f right()    const { return viewInv_.getRowAsVec3(0); }
    HKAPI constexpr hkm::vec3f top()      const { return viewInv_.getRowAsVec3(1); }
    HKAPI constexpr hkm::vec3f forward()  const { return viewInv_.getRowAsVec3(2); }
    HKAPI constexpr hkm::vec3f position() const { return viewInv_.getRowAsVec3(3); }

    HKAPI constexpr hkm::mat4f view() const { return view_; }
    HKAPI constexpr hkm::mat4f projection() const { return proj_; }
    HKAPI constexpr hkm::mat4f viewProjection() const { return viewProj_; }

    HKAPI constexpr hkm::mat4f viewInv() const { return viewInv_; }
    HKAPI constexpr hkm::mat4f projectionInv() const { return projInv_; }
    HKAPI constexpr hkm::mat4f viewProjectionInv() const { return viewProjInv_; }

    HKAPI constexpr f32 fov() const { return fov_; }
    HKAPI constexpr f32 aspect() const { return aspectRatio_; }

private:
    hkm::vec3f& pos()
    {
        updated = false;
        return viewInv_.getRowAsVec3(3);
    }

private:
    f32 fov_ = 0;
    f32 aspectRatio_ = 0;

    hkm::mat4f view_ = hkm::mat4f::identity();
    hkm::mat4f proj_ = hkm::mat4f::identity();
    hkm::mat4f viewProj_ = hkm::mat4f::identity();

    hkm::mat4f viewInv_ = hkm::mat4f::identity();
    hkm::mat4f projInv_ = hkm::mat4f::identity();
    hkm::mat4f viewProjInv_ = hkm::mat4f::identity();

    hkm::quaternion rotation_ = hkm::quaternion::identity();

    b8 updated = false;
};

#endif // HK_CAMERA_H
