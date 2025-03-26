#ifndef HK_CAMERA_H
#define HK_CAMERA_H

#include "hkcommon.h"

#include "hkstl/math/hkmath.h"

namespace hk {

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

    HKAPI void setWorldRotation(const hkm::quaternion &rotation);
    HKAPI void addWorldRotation(const hkm::quaternion &rotation);

    HKAPI void update();

public:
    HKAPI constexpr hkm::vec3f right()    const { return view_inv_.getRowAsVec3(0); }
    HKAPI constexpr hkm::vec3f top()      const { return view_inv_.getRowAsVec3(1); }
    HKAPI constexpr hkm::vec3f forward()  const { return view_inv_.getRowAsVec3(2); }
    HKAPI constexpr hkm::vec3f position() const { return view_inv_.getRowAsVec3(3); }

    HKAPI constexpr hkm::mat4f view() const { return view_; }
    HKAPI constexpr hkm::mat4f projection() const { return proj_; }
    HKAPI constexpr hkm::mat4f viewProjection() const { return view_proj_; }

    HKAPI constexpr hkm::mat4f viewInv() const { return view_inv_; }
    HKAPI constexpr hkm::mat4f projectionInv() const { return proj_inv_; }
    HKAPI constexpr hkm::mat4f viewProjectionInv() const { return view_proj_inv_; }

    HKAPI constexpr f32 fov() const { return fov_; }
    HKAPI constexpr f32 aspect() const { return aspect_ratio_; }
    HKAPI constexpr f32 far() const { return far_; }
    HKAPI constexpr f32 near() const { return near_; }

private:
    hkm::vec3f& pos()
    {
        updated = false;
        return view_inv_.getRowAsVec3(3);
    }

private:
    f32 fov_ = 0;
    f32 aspect_ratio_ = 0;
    f32 near_ = 0;
    f32 far_ = 0;

    hkm::mat4f view_ = hkm::mat4f::identity();
    hkm::mat4f proj_ = hkm::mat4f::identity();
    hkm::mat4f view_proj_ = hkm::mat4f::identity();

    hkm::mat4f view_inv_ = hkm::mat4f::identity();
    hkm::mat4f proj_inv_ = hkm::mat4f::identity();
    hkm::mat4f view_proj_inv_ = hkm::mat4f::identity();

    hkm::quaternion rotation_ = hkm::quaternion::identity();

    b8 updated = false;
};

}

#endif // HK_CAMERA_H
