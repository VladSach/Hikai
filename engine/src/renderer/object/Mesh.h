#ifndef HK_MESH_H
#define HK_MESH_H

#include "Vertex.h"
#include "Transform.h"

#include "utils/containers/hkvector.h"

namespace hk {

struct Mesh {
    hk::vector<Vertex> vertices;
    hk::vector<u32> indices;

    struct triangle { u32 indices[3]; };
    hk::vector<triangle> triangles;

    hk::vector<Transform> instances;
    // hk::vector<hkm::mat4f> instancesInv;

};

}

#endif // HK_MESH_H
