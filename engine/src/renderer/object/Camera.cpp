#include "Camera.h"

#include <cmath>

namespace hk {

void Camera::init(f32 fov, f32 aspect_ratio, f32 near_plane, f32 far_plane)
{
    fov_ = fov;
    aspect_ratio_ = aspect_ratio;
    near_ = near_plane;
    far_ = far_plane;

    f32 scale = 1.f / std::tanf(fov * .5f * hkm::degree2rad);

    f32 remap_z1 = near_ / (near_ - far_);
    f32 remap_z2 = far_ * near_ / (near_ - far_);

    proj_ = { scale/aspect_ratio_,  0.f,    0.f,       0.f,
              0.f,                 -scale,  0.f,       0.f,
              0.f,                  0.f,    remap_z1,  1.f,
              0.f,                  0.f,   -remap_z2,  0.f };

    proj_inv_ = inverse(proj_);
}

void Camera::deinit()
{
    fov_ = 0;
    aspect_ratio_ = 0;
    near_ = 0;
    far_ = 0;

    view_      = mat4f::identity();
    proj_      = mat4f::identity();
    view_proj_ = mat4f::identity();

    view_inv_      = mat4f::identity();
    proj_inv_      = mat4f::identity();
    view_proj_inv_ = mat4f::identity();

    rotation_ = mat3f::identity();

    updated_ = false;
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
    view_inv_.asMat3(rotation_.rotmat());

    view_ = inverse(view_inv_);

    view_proj_ = view_ * proj_;
    view_proj_inv_ = proj_inv_ * view_inv_;

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
    pos() += offset.x * right() + offset.y * top() + offset.z * forward();
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

void Camera::setWorldRotation(const hkm::quaternion &rotation)
{
    updated = false;

    rotation_ = normalize(rotation);
}

void Camera::addWorldRotation(const hkm::quaternion &rotation)
{
    updated = false;

    rotation_ = normalize(rotation_ * rotation);
}

}
