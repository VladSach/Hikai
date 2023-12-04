#ifndef HK_RENDERER_BACKEND_H
#define HK_RENDERER_BACKEND_H

#include "defines.h"
#include "platform/platform.h"

class Backend {
public:
    virtual void init(const Window &window) = 0;
    virtual void deinit() = 0;
    virtual void draw() = 0;
};

#endif // HK_RENDERER_BACKEND_H
