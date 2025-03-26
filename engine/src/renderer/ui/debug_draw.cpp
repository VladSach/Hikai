#include "debug_draw.h"

#include "renderer/hkvulkan.h"

// FIX: temp
#include "imguidebug.h"

// FIX: change to creating shaders here, without assets
#include "resources/AssetManager.h"

namespace hk::dd {

struct DebugShape {
    ShapeDesc info;

    b8 point_array = false;
    // PERF: change to vertecies + indices if memory will become a problem
    hk::vector<hkm::vec3f> points = {};

    // FIX: temp
    VkDescriptorSet descriptor_set;
};

static struct DebugContext {
    hk::vector<DebugShape> shapes = {};

    BufferHandle buf; // contains points from all shapes

    // Config
    b8 wide_lines;
    b8 large_points;
    hkm::vec2f line_range;
    hkm::vec2f point_range;

    // Draw resources
    hk::Pipeline line_pipeline;
    hk::Pipeline point_pipeline;
    hk::DescriptorLayout set_layout;
    u32 hndl_vertex;
    u32 hndl_pixel;
} ctx;

void init(VkDescriptorSetLayout global_set_layout,
          VkDescriptorSetLayout pass_set_layout,
          VkRenderPass pass)
{
    /* ===== Get Config ===== */
    auto &info = hk::vkc::adapter_info();

    ctx.wide_lines = info.features.core.wideLines;
    ctx.large_points = info.features.core.largePoints;

    ctx.line_range = info.properties.limits.lineWidthRange;
    ctx.point_range = info.properties.limits.pointSizeRange;

    /* ===== Create Vertex Buffer ===== */
    BufferDesc buffer_desc = {};
    buffer_desc.type = BufferType::VERTEX_BUFFER;
    // buffer_desc.access = MemoryType::GPU_LOCAL;
    buffer_desc.access = MemoryType::CPU_UPLOAD;
    buffer_desc.size = 16;
    buffer_desc.stride = sizeof(hkm::vec4f);
    ctx.buf = bkr::create_buffer(buffer_desc, "Draw Debug Data");

    /* ===== Init Renderer Resources ===== */
    hk::PipelineBuilder builder;

    const std::string path = "..\\engine\\assets\\shaders\\";

    hk::dxc::ShaderDesc desc;
    desc.entry = "main";
    desc.type = ShaderType::Vertex;
    desc.model = ShaderModel::SM_6_0;
    desc.ir = ShaderIR::SPIRV;
#ifdef HKDEBUG
    desc.debug = true;
#else
    desc.debug = false;
#endif

    desc.path = path + "Debug.vert.hlsl";
    ctx.hndl_vertex = hk::assets()->load(desc.path, &desc);

    desc.type = ShaderType::Pixel;
    desc.path = path + "Debug.frag.hlsl";
    ctx.hndl_pixel = hk::assets()->load(desc.path, &desc);

    builder.setShader(ctx.hndl_vertex);
    builder.setShader(ctx.hndl_pixel);

    hk::vector<Format> vert_layout = {
        // position
        hk::Format::R32G32B32_SFLOAT,
    };
    builder.setVertexLayout(sizeof(hkm::vec3f), vert_layout);

    builder.setRasterizer(VK_POLYGON_MODE_FILL,
                          VK_CULL_MODE_FRONT_BIT,
                          VK_FRONT_FACE_COUNTER_CLOCKWISE);
    builder.setMultisampling();

    ctx.set_layout.init(hk::DescriptorLayout::Builder()
        .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
        .build()
    );
    hk::debug::setName(ctx.set_layout.handle(), "Debug Descriptor Layout");

    builder.setPushConstants({{ VK_SHADER_STAGE_ALL_GRAPHICS, 0, 64 }});

    hk::vector<VkDescriptorSetLayout> set_layouts = {
        global_set_layout,
        pass_set_layout,
    };
    builder.setDescriptors(set_layouts);

    builder.setDepthStencil(VK_FALSE, VK_COMPARE_OP_NEVER, VK_FORMAT_UNDEFINED);
    builder.setColors({{ VK_FORMAT_R16G16B16A16_SFLOAT, hk::BlendState::NONE }});

    hk::vector<VkDynamicState> dynamic_states = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
        VK_DYNAMIC_STATE_LINE_WIDTH
    };
    builder.setDynamicStates(dynamic_states);

    // FIX: ?maybe create separate pass for debug
    builder.setName("Debug Point");
    builder.setInputTopology(VK_PRIMITIVE_TOPOLOGY_POINT_LIST);
    ctx.point_pipeline = builder.build(pass, 1);

    builder.setName("Debug Line");
    builder.setInputTopology(VK_PRIMITIVE_TOPOLOGY_LINE_LIST);
    ctx.line_pipeline = builder.build(pass, 1);
}

void deinit()
{
    bkr::destroy_buffer(ctx.buf);
    ctx.shapes.clear();

    ctx.line_pipeline.deinit();
    ctx.point_pipeline.deinit();
    ctx.set_layout.deinit();
}

void draw(VkCommandBuffer cmd)
{
    if (!ctx.shapes.size()) { return; }

    hk::imgui::debug::push(ctx.point_pipeline);
    hk::imgui::debug::push(ctx.line_pipeline);

    hk::vector<DebugShape> point_shapes = {};
    hk::vector<DebugShape> line_shapes = {};

    // Sort
    for (auto &shape : ctx.shapes) {
        if (shape.point_array) {
            point_shapes.push_back(shape);
        } else {
            line_shapes.push_back(shape);
        }
    }

    // Update buffer
    hk::vector<hkm::vec3f> points;

    for (auto &shape : point_shapes) {
        for (u32 i = 0; i < shape.points.size(); ++i) {
            points.push_back(shape.points[i]);
        }

        shape.info.thickness = ctx.large_points ?
            hkm::clamp(shape.info.thickness,
                       ctx.point_range.x,
                       ctx.point_range.y) : 1.f;
    }

    for (auto &shape : line_shapes) {
        for (u32 i = 0; i < shape.points.size(); ++i) {
            points.push_back(shape.points[i]);
        }

        shape.info.thickness = ctx.wide_lines ?
            hkm::clamp(shape.info.thickness,
                       ctx.line_range.x,
                       ctx.line_range.y) : 1.f;
    }

    // Resize buffer if needed
    if (bkr::desc(ctx.buf).size <= points.size()) {
        bkr::resize_buffer(ctx.buf, points.size());
    }

    bkr::update_buffer(ctx.buf, points.data());

    // Draw
    u32 offset = 0;
    bkr::bind_buffer(ctx.buf, cmd);

    // Draw point shapes
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx.point_pipeline.handle());

    for (auto &shape : point_shapes) {
        vkCmdPushConstants(cmd, ctx.point_pipeline.layout(),
                           VK_SHADER_STAGE_ALL_GRAPHICS, 0,
                           sizeof(ShapeDesc), &shape.info);

        vkCmdDraw(cmd, shape.points.size(), 1, offset, 0);
        offset += shape.points.size();
    }

    // Draw line shapes
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx.line_pipeline.handle());

    for (auto &shape : line_shapes) {
        vkCmdPushConstants(cmd, ctx.line_pipeline.layout(),
                           VK_SHADER_STAGE_ALL_GRAPHICS, 0,
                           sizeof(ShapeDesc), &shape.info);

        vkCmdSetLineWidth(cmd, shape.info.thickness);
        vkCmdDraw(cmd, shape.points.size(), 1, offset, 0);
        offset += shape.points.size();
    }

    // FIX: remove
    // u32 shapes_amount = ctx.shapes.size();
    // u32 buf_size = ctx.buf.size();
    // u32 buf_stride = ctx.buf.stride();
    // u64 buf_mem_size = ctx.buf.memsize();
    // hk::imgui::push([=](){
    //     if (ImGui::Begin("Draw Debug")) {
    //         ImGui::Text("All Shapes: %i", shapes_amount);
    //         ImGui::Text("Lines Shapes: %i", line_shapes.size());
    //         // ImGui::Text("Lines: %i", lines_points.size() / 2);
    //
    //         ImGui::Text("Buffer Size: %i", buf_size);
    //         ImGui::Text("Buffer Stride: %i", buf_stride);
    //         ImGui::Text("Buffer Mem Size: %llu", buf_mem_size);
    //
    //         ImGui::Text("Offset: %i", offset);
    //     } ImGui::End();
    // });

    ctx.shapes.clear();
}

/* ===== Internal Shapes ===== */
constexpr void point(DebugShape &out, const hkm::vec3f &pos)
{
    out.point_array = true;
    out.points.push_back(pos);
}

constexpr void line(DebugShape &out,
                    const hkm::vec3f &from, const hkm::vec3f &to)
{
    out.point_array = false;
    out.points.push_back(from);
    out.points.push_back(to);
}

inline void rect(DebugShape &out,
                 const hkm::vec3f &p1, const hkm::vec3f &p2,
                 const hkm::vec3f &p3, const hkm::vec3f &p4,
                 const hkm::vec3f &normal)
{
    hkm::vec3f ab = p2 - p1;
    hkm::vec3f ac = p3 - p1;
    hkm::vec3f ad = p4 - p1;

    // ALWAYS_ASSERT(!dot(ad, cross(ab, ac)), "Points are not coplanar");

    // Cross product between edges should align with normal for clockwise order
    // ALWAYS_ASSERT(dot(normal, cross(ab, ac)) < 0.f, "Points are not clockwise");

    line(out, p1, p2);
    line(out, p2, p3);
    line(out, p3, p4);
    line(out, p4, p1);
}

/* With small sectors values also draws
 * regular(both equilateral and equiangular) polygons,
 * 3 is a triangle, 4 is a square, 5 is a pentagon, etc */
inline void circle(DebugShape &out,
                   const hkm::vec3f &center, f32 radius,
                   const hkm::vec3f &normal, u32 sectors)
{
    // TODO: move into math utils
    // Find basis
    const hkm::vec3f norm = normalize(normal);
    hkm::vec3f e1 = (std::abs(norm.x) > .9f) ?
        hkm::vec3f(0, 1, 0) : hkm::vec3f(1, 0, 0);

    // Project e1 onto the circle's plane
    e1 = e1 - norm * (dot(e1, norm));
    e1 = normalize(e1);
    hkm::vec3f e2 = cross(norm, e1);

    // line(circle, center, center + e1 * .2f),
    // line(circle, center, center + e2 * .2f),

    f32 step_angle = hkm::tau / sectors;

    hkm::vec3f point;
    hkm::vec3f prev = center + e1 * radius;
    for (u32 i = 1; i <= sectors; ++i) {
        f32 cur_angle = i * step_angle;

        point = center;
        point += e1 * radius * std::cos(cur_angle);
        point += e2 * radius * std::sin(cur_angle);

        line(out, prev, point);
        prev = point;
    }
}

inline void sphere(DebugShape &out, const hkm::vec3f &center, f32 radius)
{
    /* TODO:
     * add option to choose between point and wireframe,
     * add N input (points or sectors?)
     */

    // constexpr u32 sectors = 24;
    //
    // // https://github.com/glampert/debug-draw/blob/dd78c2230adce80ab2e1e257e2e257cb25ea1312/debug_draw.hpp#L2971
    // constexpr f32 step_angle = hkm::tau / sectors; // angle between lines in rad
    //
    // hk::vector<hkm::vec3f> cache(sectors, center + hkm::vec3f(0.f, radius, 0.f));
    //
    // hkm::vec3f prev, curr;
    // for (f32 lat = step_angle; lat <= hkm::tau; lat += step_angle) {
    //     f32 lat_sin = std::sin(lat);
    //     f32 lat_cos = std::cos(lat);
    //
    //     prev = {
    //         center.x,
    //         center.y + radius * lat_cos,
    //         center.z + radius * lat_sin,
    //     };
    //
    //     u32 n = 0;
    //     for (f32 lon = step_angle; lon <= hkm::tau; lon += step_angle, ++n) {
    //         curr = {
    //             center.x + radius * std::sin(lon) * lat_sin,
    //             prev.y,
    //             center.z + radius * std::cos(lon) * lat_sin,
    //         };
    //
    //         line(out, prev, curr);
    //         line(out, prev, cache[n]);
    //
    //         cache[n] = prev;
    //         prev = curr;
    //     }
    // }

    // Spherical Fibonacci Lattice
    constexpr u32 n = 5000;
    constexpr f32 eps = 0.f;

    // Add points at poles (if epsilon => 10)
    // point(sphere, { center.x, center.y + radius, center.z });
    // point(sphere, { center.x, center.y - radius, center.z });

    for (u32 i = 0; i < n - 2; ++i) {
        f32 theta = hkm::tau * i / hkm::phi; // or lat
        f32 phi = std::acos(1.f - 2.f * (i + eps) / (n - 1.f + 2.f * eps)); // or lon
        // Less uniform version
        // f32 theta = 2.f * hkm::phi * i;
        // f32 phi = acos(1.f - (2.f * i) / n);

        point(out, {
            center.x + radius * std::sin(phi) * std::cos(theta),
            center.y + radius * std::cos(phi),
            center.z + radius * std::sin(phi) * std::sin(theta)
        });
    }
}

/* ===== User Shapes ===== */
void point(const ShapeDesc &desc, const hkm::vec3f &pos)
{
    ctx.shapes.push_back({ desc, true, { pos } });
}

void line(const ShapeDesc &desc, const hkm::vec3f &from, const hkm::vec3f &to)
{
    ctx.shapes.push_back({ desc, false, { from, to }});
}

void rect(const ShapeDesc &desc,
          const hkm::vec3f &p1, const hkm::vec3f &p2,
          const hkm::vec3f &p3, const hkm::vec3f &p4,
          const hkm::vec3f &normal)
{
    DebugShape rect;
    rect.info = desc;

    hk::dd::rect(rect, p1, p2, p3, p4, normal);

    ctx.shapes.push_back(rect);
}

void circle(const ShapeDesc &desc,
            const hkm::vec3f &center, f32 radius,
            const hkm::vec3f &normal)
{
    DebugShape circle;
    circle.info = desc;

    hk::dd::circle(circle, center, radius, normal, 50);

    ctx.shapes.push_back(circle);
}

void sphere(const ShapeDesc &desc, const hkm::vec3f &center, f32 radius)
{
    DebugShape sphere;
    sphere.info = desc;

    hk::dd::sphere(sphere, center, radius);

    ctx.shapes.push_back(sphere);
}

void view_frustum(const ShapeDesc &desc, const hkm::mat4f view_proj_inv)
{
    DebugShape frustum;
    frustum.info = desc;

    hkm::vec3f planes[8] = {
        // near
        {-1,  1, 0}, // Top-left
        { 1,  1, 0}, // Top-right
        { 1, -1, 0}, // Bottom-right
        {-1, -1, 0}, // Bottom-left

        // far
        {-1,  1, 1},
        { 1,  1, 1},
        { 1, -1, 1},
        {-1, -1, 1},
    };

    hkm::vec3f out[8];
    for (u32 i = 0; i < 8; ++i) {
        out[i] = hkm::transformPoint(view_proj_inv, planes[i]);
    }

    hkm::vec3f normal = hkm::transformVec(view_proj_inv, {0, 0, 1});
    rect(frustum, out[0], out[1], out[2], out[3], normal);
    rect(frustum, out[4], out[5], out[6], out[7], normal);

    line(frustum, out[0], out[4]);
    line(frustum, out[1], out[5]);
    line(frustum, out[2], out[6]);
    line(frustum, out[3], out[7]);

    ctx.shapes.push_back(frustum);
}

void conical_frustum(const ShapeDesc &desc,
                     const hkm::vec3f &apex, f32 apex_radius,
                     const hkm::vec3f &base, f32 base_radius)
{
    DebugShape frustum;
    frustum.info = desc;

    hkm::vec3f dir = normalize(base - apex);

    static u32 n = 80;
    constexpr u32 ceiling = 1000;
    // smaller then 30 doesn't really make sense
    n = hkm::clamp(n, 30u, ceiling);

    circle(frustum, apex, apex_radius, dir, n);
    circle(frustum, base, base_radius, dir, n);

    static u32 modifier = 8; // draw only every M-th line
    // [1, n * 2] => [draw all lines, draw one line]
    modifier = hkm::clamp(modifier, 1u, n * 2u);

    // FIX: (test) multiple by two only if NOT point array
    for (u32 i = 0; i < n * 2; i += modifier * 2) {
        // fan
        // line(frustum, frustum.points.at(i), frustum.points.at(i + n));

        line(frustum, frustum.points.at(i), frustum.points.at(i + n * 2));
    }

    ctx.shapes.push_back(frustum);
}

}
