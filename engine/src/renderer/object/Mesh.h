#ifndef HK_MESH_H
#define HK_MESH_H

#include "Vertex.h"
#include "utils/containers/hkvector.h"

namespace hk {

struct Mesh {
    hk::vector<Vertex> vertices;
    hk::vector<u32> indices;
};

}

#endif // HK_MESH_H
