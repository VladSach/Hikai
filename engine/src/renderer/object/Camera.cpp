#include "Camera.h"

#include <cmath>

void Camera::setPerspective(f32 fov, f32 aspectRatio, f32 nearPlane, f32 farPlane)
{
    constexpr f32 to_radians = 3.14f / 180;
    f32 scale = 1/std::tanf(fov * .5f * to_radians);

    f32 remap_z1 = nearPlane / (nearPlane - farPlane);
    f32 remap_z2 = farPlane * nearPlane / (nearPlane - farPlane);

    proj_ = { scale/aspectRatio,  0.f,    0.f,       0.f,
              0.f,               -scale,  0.f,       0.f,
              0.f,                0.f,    remap_z1,  1.f,
              0.f,                0.f,   -remap_z2,  0.f };

    projInv_ = inverse(proj_);

    fov_ = fov;
    aspectRatio_ = aspectRatio;
}

void Camera::lookAt(const hkm::vec3f &dir)
{
    f32 dot = hkm::dot(forward(), dir);

    f32 angle = std::acos(dot);
    hkm::vec3f axis = normalize(cross(forward(), dir));

    rotation_ = hkm::fromAxisAngle(axis, angle);
    rotation_ = normalize(rotation_);
}

void Camera::fixedBottomRotation(const hkm::vec2f &angles)
{
    updated = false;

    hkm::vec2f rads = angles * hkm::degree2rad;

    hkm::quaternion yaw = hkm::fromAxisAngle({ 0.f, 1.f, 0.f}, rads.x);
    hkm::quaternion pitch = hkm::fromAxisAngle({1.f, 0.f, 0.f}, rads.y);

    rotation_ = yaw * rotation_;
    rotation_ = rotation_ * pitch;

    rotation_ = normalize(rotation_);
}

void Camera::update()
{
    if(updated) return;

    rotation_ = normalize(rotation_);
    viewInv_.asMat3(rotation_.rotmat());

    view_ = inverse(viewInv_);

    viewProj_ = view_ * proj_;
    viewProjInv_ = viewInv_ * projInv_;

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

    hkm::vec3f rads = angles * hkm::degree2rad;

    rotation_ =             hkm::fromAxisAngle({0.f, 0.f, 1.f}, rads.x);
    rotation_ = rotation_ * hkm::fromAxisAngle({1.f, 0.f, 0.f}, rads.y);
    rotation_ = rotation_ * hkm::fromAxisAngle({0.f, 1.f, 0.f}, rads.z);

    rotation_ = normalize(rotation_);
}

void Camera::addWorldAngles(const hkm::vec3f &angles)
{
    updated = false;

    hkm::vec3f rads = angles * hkm::degree2rad;

    rotation_ = rotation_ * hkm::fromAxisAngle({0.f, 1.f, 0.f}, rads.x);
    rotation_ = rotation_ * hkm::fromAxisAngle({1.f, 0.f, 0.f}, rads.y);
    rotation_ = rotation_ * hkm::fromAxisAngle({0.f, 0.f, 1.f}, rads.z);

    rotation_ = normalize(rotation_);
}

void Camera::addRelativeAngles(const hkm::vec3f &angles)
{
    updated = false;

    hkm::vec3f rads = angles * hkm::degree2rad;

    rotation_ = rotation_ * hkm::fromAxisAngle(top(), rads.x);
    rotation_ = rotation_ * hkm::fromAxisAngle(right(), rads.y);
    rotation_ = rotation_ * hkm::fromAxisAngle(forward(), rads.z);

    rotation_ = normalize(rotation_);
}
