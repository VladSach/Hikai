#ifndef HK_MODEL_MANAGER_H
#define HK_MODEL_MANAGER_H

#include "defines.h"

struct Model {
    u32 dummy;
};

namespace hk::loader {

void init();
void deinit();

Model* loadModel(const std::string &path);

}

#endif // HK_MODEL_MANAGER_H
