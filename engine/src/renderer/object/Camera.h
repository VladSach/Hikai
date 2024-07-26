#ifndef HK_CAMERA_H
#define HK_CAMERA_H

#include "math/hkmath.h"

class Camera {
public:
    void setPerspective(f32 fov, f32 aspect, f32 nearPlane, f32 farPlane);
    void lookAt(const hkm::vec3f &direction);

    void setWorldOffset(const hkm::vec3f &offset);
    void addWorldOffset(const hkm::vec3f &offset);
    void addRelativeOffset(const hkm::vec3f &offset);

    // FIX: they don't work as expected when there multiple angles
    void setWorldAngles(const hkm::vec3f &angles);
    void addWorldAngles(const hkm::vec3f &angles);
    void addRelativeAngles(const hkm::vec3f &angles);

    void update();

public:
    constexpr hkm::vec3f right()    const { return viewInv.getRowAsVec3(0); }
    constexpr hkm::vec3f top()      const { return viewInv.getRowAsVec3(1); }
    constexpr hkm::vec3f forward()  const { return viewInv.getRowAsVec3(2); }
    constexpr hkm::vec3f position() const { return viewInv.getRowAsVec3(3); }

    constexpr hkm::mat4f getView() const { return view; }
    constexpr hkm::mat4f getProjection() const { return proj; }
    constexpr hkm::mat4f getViewProjection() const { return viewProj; }

    constexpr hkm::mat4f getInvViewProjection() const { return viewProjInv; }

private:
    hkm::vec3f& pos()
    {
        updated = false;
        return viewInv.getRowAsVec3(3);
    }

private:
    f32 fov_ = 0;
    f32 aspectRatio_ = 0;

    hkm::mat4f view = hkm::mat4f::identity();
    hkm::mat4f proj = hkm::mat4f::identity();
    hkm::mat4f viewProj = hkm::mat4f::identity();

    hkm::mat4f viewInv = hkm::mat4f::identity();
    hkm::mat4f projInv = hkm::mat4f::identity();
    hkm::mat4f viewProjInv = hkm::mat4f::identity();

    hkm::quaternion rotation = hkm::quaternion::identity();

    b8 updated = false;
};

#endif // HK_CAMERA_H
