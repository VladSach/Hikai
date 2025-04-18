#ifndef HK_SHADER_LOADER_H
#define HK_SHADER_LOADER_H

#include "hkstl/containers/hkvector.h"

#include <string>

// TODO: I don't like them being in global space
enum class ShaderType : u8 {
    None = 0,

    Vertex,
    Hull,
    Domain,
    Geometry,
    Pixel,

    Compute,

    Max
};

enum class ShaderModel : u8 {
    SM_6_0 = 0,
    SM_6_1,
    SM_6_2,
    SM_6_3,
    SM_6_4,
    SM_6_5,
    SM_6_6,
    SM_6_7,
};

enum class ShaderIR {
    DXIL,
    SPIRV,
};

namespace hk::dxc {

// TODO: things that should be passed from outside code
// defines
// include directories
struct ShaderDesc {
    std::string path;

    std::string entry = "main";
    ShaderType type = ShaderType::Vertex;
    ShaderModel model = ShaderModel::SM_6_0;

    // Intermediate Representation
    ShaderIR ir = ShaderIR::SPIRV;
    b8 debug = false;
};

void init();
void deinit();

hk::vector<u32> loadShader(const ShaderDesc &desc);

}

#endif // HK_SHADER_LOADER_H
