#include "PlatformArgs.h"

PlatformArgs *PlatformArgs::singleton = nullptr;

PlatformArgs *PlatformArgs::instance()
{
    if (singleton == nullptr) {
        singleton = new PlatformArgs();
    }
    return singleton;
}
