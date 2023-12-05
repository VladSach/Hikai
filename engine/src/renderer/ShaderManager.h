#ifndef HK_SHADER_MANAGER_H
#define HK_SHADER_MANAGER_H

#include "defines.h"
#include "utils/containers/hkvector.h"

#include <string>

class ShaderManager {

public:
    hk::vector<u32> loadShader(const std::string &path, const b8 type);
};

#endif // HK_SHADER_MANAGER_H
