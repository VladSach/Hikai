#include "ModelLoader.h"

// TODO: replace with Hikai implementation
#include <unordered_map>

static std::unordered_map<std::string, Model*> loaded_;

Model* loadModel(const std::string &path)
{
    if (loaded_.count(path)) {
        return loaded_.at(path);
    }
}
