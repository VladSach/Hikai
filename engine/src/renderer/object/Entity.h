#ifndef HK_ENTITY_H
#define HK_ENTITY_H

#include "Transform.h"

#include <string>

namespace hk {

struct Entity {
    std::string name_;
    Transform transform_;
};

}

#endif // HK_ENTITY_H
