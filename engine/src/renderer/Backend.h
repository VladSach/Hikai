#ifndef HK_RENDERER_BACKEND_H
#define HK_RENDERER_BACKEND_H

#include "defines.h"

class Backend {
public:
    virtual void init() = 0;
    virtual void deinit() = 0;
};

#endif // HK_RENDERER_BACKEND_H
