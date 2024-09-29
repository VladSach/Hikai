#ifndef HK_ASSET_MANAGER_H
#define HK_ASSET_MANAGER_H

#include "Asset.h"

#include <functional>

namespace hk {

class AssetManager {
public:
    void init(const std::string &folder);
    void deinit();

    HKAPI u32 load(const std::string &path, void *data = nullptr);
    HKAPI u32 load(const std::string &path, Asset::Type type, void *data = nullptr);
    HKAPI void unload(u32 handle);

    void reload(u32 handle);

    HKAPI void attachCallback(u32 handle, std::function<void()> callback);


    inline std::string getAssetPath(u32 handle)
    {
        return assets_.at(getIndex(handle))->path;
    }

    // TODO: probably don't want to get access to user to Assets
    Asset* get(u32 handle) const
    {
        return assets_.at(getIndex(handle));
    }

    // FIX: temp
    const hk::ShaderAsset& getShader(u32 handle) const
    {
        return *static_cast<hk::ShaderAsset*>(assets_.at(getIndex(handle)));
    }

    const hk::TextureAsset& getTexture(u32 handle) const
    {
        return *static_cast<hk::TextureAsset*>(assets_.at(getIndex(handle)));
    }

public:
    inline std::string folder() const { return folder_; }

    // FIX: temp
    const hk::vector<Asset*> assets() const { return assets_; }

private:
    u32 loadTexture(const std::string &path);
    u32 loadShader(const std::string &path, void *data);

private:
    std::string folder_ = "assets\\";

    u32 index_ = 0;
    // TODO: replace with a memory pool
    hk::vector<Asset*> assets_;

    // TODO: change vector of callbacks to linked list
    hk::vector<hk::vector<std::function<void()>>> callbacks_;

    // PERF: rethink usage of unordered_map. why even use handles,
    // if forced to use map nonetheless
    std::unordered_map<std::string, u32> paths_;
};

HKAPI AssetManager *assets();

}

#endif // HK_ASSET_MANAGER_H
