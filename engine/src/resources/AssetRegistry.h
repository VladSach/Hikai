#ifndef HK_ASSET_REGISTRY_H
#define HK_ASSET_REGISTRY_H

#include "defines.h"

#include "utils/containers/hkvector.h"

namespace hk {

struct AssetInfo {
    std::string path;
    u32 handle;
    b8 loaded;
    // time last_modified;
};

class AssetRegistry {
public:
    void init();
    void deinit();

    void append();
    void remove();

    void path(u32 handle);
    void handle(std::string path);

private:
    std::string folder_ = "assets\\";

    // path to handle
    std::unordered_map<std::string, u32> paths_;
    // handle to path
    std::unordered_map<u32, std::string> handles_;
};

}

#endif // HK_ASSET_REGISTRY_H
