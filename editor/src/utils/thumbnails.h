#ifndef HK_THUMBNAILS_H
#define HK_THUMBNAILS_H

#include "hikai.h"

namespace hke::thumbnail {

void init(GUI *gui);
void deinit();

void* get(u32 handle);
// void* get(const hk::Image *image);
// void* get(const std::string name);

void remove(u32 handle);

}

#endif // HK_THUMBNAILS_H
