#include "Camera.h"

#include <math.h>

void Camera::setPerspective(f32 fov, f32 aspectRatio, f32 nearPlane, f32 farPlane)
{
    constexpr f32 to_radians = 3.14f / 180;
    f32 scale = 1/tanf(fov * .5f * to_radians);

    f32 remap_z1 = nearPlane / (nearPlane - farPlane);
    f32 remap_z2 = farPlane * nearPlane / (nearPlane - farPlane);

    proj = { scale/aspectRatio,  0.f,    0.f,       0.f,
             0.f,               -scale,  0.f,       0.f,
             0.f,                0.f,    remap_z1,  1.f,
             0.f,                0.f,   -remap_z2,  0.f };

    projInv = inverse(proj);

    fov_ = fov;
    aspectRatio_ = aspectRatio;
}

void Camera::lookAt(const hkm::vec3f &dir)
{
    f32 dot = hkm::dot(forward(), dir);

    f32 angle = acos(dot);
    hkm::vec3f axis = normalize(cross(forward(), dir));

    rotation = hkm::fromAxisAngle(axis, angle);
    rotation = normalize(rotation);
}

void Camera::update()
{
    if(updated) return;

    viewInv.asMat3(rotation.rotmat());

    view = inverse(viewInv);

    viewProj = view * proj;
    viewProjInv = projInv * viewInv;

    updated = true;
}

void Camera::setWorldOffset(const hkm::vec3f &offset)
{
    updated = false;
    pos() = offset;
}

void Camera::addWorldOffset(const hkm::vec3f &offset)
{
    updated = false;
    pos() += offset;
}

void Camera::addRelativeOffset(const hkm::vec3f &offset)
{
    updated = false;
    pos() += offset.x * right() +
             offset.y * top() +
             offset.z * forward();
}

void Camera::setWorldAngles(const hkm::vec3f &angles)
{
    updated = false;

    hkm::vec3f rads = angles * hkm::angle2rad;

    rotation =            hkm::fromAxisAngle({0.f, 0.f, 1.f}, rads.x);
    rotation = rotation * hkm::fromAxisAngle({1.f, 0.f, 0.f}, rads.y);
    rotation = rotation * hkm::fromAxisAngle({0.f, 1.f, 0.f}, rads.z);

    rotation = normalize(rotation);
}

void Camera::addWorldAngles(const hkm::vec3f &angles)
{
    updated = false;

    hkm::vec3f rads = angles * hkm::angle2rad;

    rotation = rotation * hkm::fromAxisAngle({0.f, 1.f, 0.f}, rads.x);
    rotation = rotation * hkm::fromAxisAngle({1.f, 0.f, 0.f}, rads.y);
    rotation = rotation * hkm::fromAxisAngle({0.f, 0.f, 1.f}, rads.z);

    rotation = normalize(rotation);
}

void Camera::addRelativeAngles(const hkm::vec3f &angles)
{
    updated = false;

    hkm::vec3f rads = angles * hkm::angle2rad;

    rotation = rotation * hkm::fromAxisAngle(top(), rads.x);
    rotation = rotation * hkm::fromAxisAngle(right(), rads.y);
    rotation = rotation * hkm::fromAxisAngle(forward(), rads.z);

    rotation = normalize(rotation);
}
