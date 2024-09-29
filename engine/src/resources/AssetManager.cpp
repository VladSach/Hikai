#include "AssetManager.h"

#include "loaders/ShaderLoader.h"

#include "utils/Filewatch.h"
#include "platform/platform.h"
#include "core/EventSystem.h"

namespace hk {

AssetManager *assets()
{
    static AssetManager *singletone = nullptr;

    if (singletone == nullptr) {
        singletone = new AssetManager();
    }
    return singletone;
}

void AssetManager::init(const std::string &folder)
{
    folder_ = folder;

    hk::filewatch::watch(folder_,
        [this](const std::string &path, const hk::filewatch::State state)
        {
            if (state != hk::filewatch::State::MODIFIED) { return; }
            /* TODO:
             * on create -> add to asset manager
             * on rename -> ask user whether that ok
             * on delete -> ask what to do with the asset, delete or keep/resave
             */

            LOG_INFO("Asset changed:", folder_ + path, stringFileState(state));

            reload(paths_[folder_ + path]);

            for (auto &callback : callbacks_.at(paths_[folder_ + path])) {
                if (callback) { callback(); }
            }
        }
    );
}

void AssetManager::deinit()
{
    folder_ = "assets\\";
    index_ = 0;
    assets_.clear();
    callbacks_.clear();
}

u32 AssetManager::load(const std::string &path, void *data)
{
    Asset::Type type = Asset::Type::NONE;

    u32 dotPos = path.find_last_of('.');
    std::string ext = path.substr(dotPos);

    if (ext == ".hlsl") {
        type = Asset::Type::SHADER;
    } else if (ext == ".png" || ext == ".jpg" || ext == ".jpeg") {
        type = Asset::Type::TEXTURE;
    } else {
        LOG_ERROR("Unknown file extension:", ext);
    }

    std::string out;

    if (hk::filesystem::findFile(folder_, path, &out)) {
        // Path inside folder_
    } else {
        // Path outside folder_
        out = path;
    }

    return load(out, type, data);
}

u32 AssetManager::load(const std::string &path, Asset::Type type, void *data)
{
    u32 handle = 0;

    switch (type) {
    case Asset::Type::SHADER: {
        handle = loadShader(path, data);
    } break;
    case Asset::Type::TEXTURE: {
        handle = loadTexture(path);
    } break;
    default:
        LOG_ERROR("Unknown asset type:", path);
    }

    // FIX: that's not intended behavior
    // initial file path could be from OUTSIDE of asset folder
    // then, asset copied INSIDE asset folder and added to paths_
    // OR asset written to asset folder on save(?)
    // all assets already inside asset folder on creation
    // should be loaded initialy
    paths_[path] = handle;
    LOG_INFO("path:", path);

    hk::EventContext context;
    context.u32[0] = handle;
    context.u32[1] = static_cast<u32>(type);
    hk::evesys()->fireEvent(hk::EventCode::EVENT_ASSET_LOADED, context);

    return handle;
}

void AssetManager::reload(u32 handle)
{
    Asset *asset = get(handle);

    switch(asset->type) {
    case Asset::Type::SHADER: {
        ShaderAsset *shader = reinterpret_cast<ShaderAsset*>(asset);
        shader->code = hk::dxc::loadShader(shader->desc);
        if (shader->code.size() <= 0) {
            LOG_WARN("Shader code was not reloaded");
            return;
        }
        shader->createShaderModule();
    } break;
    case Asset::Type::TEXTURE: {
        TextureAsset *texture = reinterpret_cast<TextureAsset*>(asset);
        texture->texture = hk::loader::loadImage(texture->path);
    } break;
    }
}

void AssetManager::attachCallback(u32 handle, std::function<void()> callback)
{
    if (callbacks_.size() < index_) {
        callbacks_.resize(index_);
    }

    callbacks_.at(getIndex(handle)).push_back(callback);
}

u32 AssetManager::loadTexture(const std::string &path)
{
    TextureAsset *asset = new TextureAsset();
    asset->name = path.substr(path.find_last_of("/\\") + 1);
    asset->path = path;
    asset->handle = index_;
    asset->type = Asset::Type::TEXTURE;

    asset->texture = hk::loader::loadImage(path);

    assets_.push_back(asset);
    ++index_;

    return asset->handle;
}

u32 AssetManager::loadShader(const std::string &path, void *data)
{
    hk::dxc::ShaderDesc desc = *reinterpret_cast<hk::dxc::ShaderDesc*>(data);

    const hk::vector<u32> code = hk::dxc::loadShader(desc);

    if (code.size() <= 0) {
        LOG_WARN("Shader code is empty");
        return 0;
    }

    ShaderAsset *asset = new ShaderAsset();
    asset->name = path.substr(path.find_last_of("/\\") + 1);
    asset->path = path;
    asset->handle = index_;
    asset->type = Asset::Type::SHADER;
    asset->desc = desc;
    asset->code = code;
    asset->createShaderModule();

    assets_.push_back(asset);
    ++index_;

    return asset->handle;
}

}
