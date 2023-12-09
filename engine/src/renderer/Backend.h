#ifndef HK_RENDERER_BACKEND_H
#define HK_RENDERER_BACKEND_H

#include "defines.h"
#include "math/math.h"
#include "platform/platform.h"

class Backend {
public:
    virtual void init() = 0;
    virtual void deinit() = 0;
    virtual void draw() = 0;

    virtual void setUniformBuffer(const hkm::vec2f &res, f32 time) = 0;
};

#endif // HK_RENDERER_BACKEND_H
