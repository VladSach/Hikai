#include "ModelLoader.h"

#include "utils/containers/hkvector.h"

// TODO: replace with Hikai implementation
#include <unordered_map>

#define TINYOBJLOADER_IMPLEMENTATION
#include "vendor/tinyobjloader/tiny_obj_loader.h"

// FIX: tmp
namespace std {
    template<>
    struct hash<hkm::vec3f> {
        size_t operator()(const hkm::vec3f& v) const {
            // Combine the hashes of the individual components
            size_t h1 = std::hash<float>()(v.x);
            size_t h2 = std::hash<float>()(v.y);
            size_t h3 = std::hash<float>()(v.z);

            return h1 ^ (h2 << 1) ^ (h3 >> 1);
        }
    };

    template<>
    struct hash<hkm::vec2f> {
        size_t operator()(const hkm::vec2f& v) const {
            // Combine the hashes of the individual components
            size_t h1 = std::hash<float>()(v.x);
            size_t h2 = std::hash<float>()(v.y);

            return h1 ^ (h2 << 1);
        }
    };
    template<> struct hash<Vertex> {
        size_t operator()(Vertex const &vertex) const {
            return ((hash<hkm::vec3f>()(vertex.pos) ^
                    (hash<hkm::vec3f>()(vertex.normal) << 1)) >> 1) ^
                    (hash<hkm::vec2f>()(vertex.tc) << 1);
        }
    };
}

template<typename T, typename... Rest>
void hashCombine(u32 &seed, const T &v, const Rest&... rest)
{
    seed ^= std::hash<T>{}(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    (hashCombine(seed, rest), ...);
}

namespace hk::loader {

static std::unordered_map<std::string, Model*> loaded_ = {};

Model* loadModel(const std::string &path)
{
    if (loaded_.count(path)) {
        return loaded_.at(path);
    }

    Model *model = new Model();
    hk::vector<Vertex> vertices = {};
    hk::vector<u32> indices = {};

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    b8 res = tinyobj::LoadObj(&attrib, &shapes, &materials,
                              &warn, &err, path.c_str());
    ALWAYS_ASSERT(res, warn + err);

    std::unordered_map<Vertex, u32> uniqueVertices = {};

    for (const auto &shape : shapes) {
        for (const auto &index : shape.mesh.indices) {
            Vertex vertex = {};

            if (index.vertex_index >= 0) {
                vertex.pos = {
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2],
                };
            }

            if (index.normal_index >= 0) {
                vertex.normal = {
                    attrib.normals[3 * index.normal_index + 0],
                    attrib.normals[3 * index.normal_index + 1],
                    attrib.normals[3 * index.normal_index + 2],
                };
            }

            if (index.texcoord_index >= 0) {
                vertex.tc = {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    1.f - attrib.texcoords[2 * index.texcoord_index + 1],
                };
            }

            if (uniqueVertices.count(vertex) == 0) {
                uniqueVertices[vertex] = vertices.size();
                vertices.push_back(vertex);
            }

            indices.push_back(uniqueVertices[vertex]);
        }
    }

    // vertices.clear();
    // indices.clear();
    //
    // vertices = {
    //     {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    //     {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    //     {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
    //     {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},
    //
    //     {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    //     {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    //     {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
    //     {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
    // };
    //
    // indices = {
    //     0, 1, 2, 2, 3, 0,
    //     4, 5, 6, 6, 7, 4
    // };

    model->init(vertices, indices);

    loaded_[path] = model;

    LOG_DEBUG("Loaded model:", path);
    LOG_TRACE("Verticies:", vertices.size());
    LOG_TRACE("Indices:", indices.size());
    return model;
}

}
