#ifndef HK_MODEL_LOADER_H
#define HK_MODEL_LOADER_H

#include "defines.h"
#include "renderer/object/Model.h"

namespace hk::loader {

HKAPI Model* loadModel(const std::string &path);

}

#endif // HK_MODEL_LOADER_H
