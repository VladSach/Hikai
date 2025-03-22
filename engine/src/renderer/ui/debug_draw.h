#ifndef HK_DEBUG_DRAW_H
#define HK_DEBUG_DRAW_H

#include "hkstl/math/hkmath.h"
#include "hkstl/containers/hkvector.h"

// FIX: remove render backend from header
#include "vulkan/vulkan_core.h"
#include "renderer/vkwrappers/Descriptors.h"

namespace hk::dd {

struct ShapeDesc {
    hkm::vec3f color;

    f32 thickness = 1.f;
    b8 use_depth = true;
};

void init(VkDescriptorSetLayout global_set_layout,
          VkDescriptorSetLayout pass_set_layout,
          VkRenderPass pass);
void deinit();

void draw(VkCommandBuffer cmd);

/* ===== Base Shapes ===== */
void point(const ShapeDesc &desc, const hkm::vec3f &pos);
void line(const ShapeDesc &desc, const hkm::vec3f &from, const hkm::vec3f &to);

/* ===== 2D shapes ===== */
// void rect(Info info, hkm::vec3f from, hkm::vec3f to); // plane?
void circle(const ShapeDesc &desc,
            const hkm::vec3f &center, f32 radius,
            const hkm::vec3f &normal);


/* ===== 3D shapes ===== */
// void aabb();
// void box(Info info, hkm::vec3f from, hkm::vec3f to); // rectangular prism
void sphere(const ShapeDesc &desc, const hkm::vec3f &center, f32 radius);
// void cone(const ShapeDesc &desc,
//           const hkm::vec3f &apex, const hkm::vec3f &base, f32 base_radius);


/* ===== Other Shapes ===== */
// void view_frustum(); // for camera
// void pyramidal_frustum(); // for rect spot light

// for disc(circle) spot light
void conical_frustum(const ShapeDesc &desc,
                     const hkm::vec3f &apex, f32 apex_radius,
                     const hkm::vec3f &base, f32 base_radius);

}

#endif // HK_DEBUG_DRAW_H
