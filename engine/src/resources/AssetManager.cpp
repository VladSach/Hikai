#include "AssetManager.h"

#include "loaders/ShaderLoader.h"
#include "loaders/ModelLoader.h"

#include "core/events.h"
#include "platform/platform.h"
#include "platform/filesystem.h"

#include "hkstl/Filewatch.h"
#include "utils/to_string.h"

#include <algorithm>

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

            LOG_INFO("Asset changed:", folder_ + path, to_string(state));

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

    for (auto &asset : assets_) {
        switch(asset->type) {
        case Asset::Type::TEXTURE: {
            TextureAsset *texture = reinterpret_cast<TextureAsset*>(asset);
            hk::bkr::destroy_image(texture->image);
        } break;
        case Asset::Type::SHADER: {
            reinterpret_cast<ShaderAsset*>(asset)->deinit();
        } break;

        default: break;
        }
    }

    assets_.clear();
    callbacks_.clear();
}

u32 AssetManager::create(Asset::Type type, void *data)
{
    u32 handle;

    switch(type) {
    case Asset::Type::MESH: {
        handle = createMesh(data);
    } break;
    case Asset::Type::MATERIAL: {
        handle = createMaterial(data);
    } break;
    default: {
        ALWAYS_ASSERT(0);
    }
    }

    hk::event::EventContext context;
    context.u32[0] = handle;
    context.u32[1] = static_cast<u32>(type);
    hk::event::fire(hk::event::EVENT_ASSET_LOADED, context);

    return handle;
}

u32 AssetManager::load(const std::string &path, void *data)
{
    std::string out;

    if (hk::filesystem::find_file(folder_, path, &out)) {
        // Path inside folder_
    } else {
        // Path outside folder_

        // Check if path exist at all
        if (!hk::filesystem::exists(path)) {
            // Try to locate file by name inside folder_
            std::string name = path.substr(path.find_last_of("/\\") + 1);
            if (hk::filesystem::find_file(folder_, name, &out)) {
                // found
            } else if (false) { // Not inside folder_ or has other name
                // TODO: add other possible ways to find file

            } else {
                LOG_WARN("File doesn't exists:", path);
                ALWAYS_ASSERT(0, "File doesn't exists:", path); // TODO: add fallback
                return 0;
            }
        } else {
            out = path;
        }

        std::string file_path = out.substr(0, out.find_last_of("/\\") + 1);
        b8 found = false;
        for (auto &watch : watched_) {
            if (watch == file_path) {
                found = true;
                break;
            }
        }

        if (!found) {
            watched_.push_back(file_path);
            hk::filewatch::watch(file_path,
                [this, file_path](const std::string &path, const hk::filewatch::State state)
                {
                    if (state != hk::filewatch::State::MODIFIED) { return; }

                    LOG_INFO("Asset changed:", file_path + path, to_string(state));

                    reload(paths_[file_path + path]);

                    for (auto &callback : callbacks_.at(paths_[file_path + path])) {
                        if (callback) { callback(); }
                    }
                }
            );
        }
    }

    if (paths_.find(out) != paths_.end()) {
        return paths_[out];
    }

    Asset::Type type = Asset::Type::NONE;

    u64 dotPos = path.find_last_of('.');
    std::string ext = path.substr(dotPos);
    std::transform(ext.begin(), ext.end(), ext.begin(),
        [](unsigned char c){ return std::tolower(c); });

    if (ext == ".hlsl") {
        type = Asset::Type::SHADER;
    } else if (
        ext == ".png"  ||
        ext == ".jpg"  ||
        ext == ".jpeg" ||
        ext == ".tga")
    {
        type = Asset::Type::TEXTURE;
    } else if (
        ext == ".3ds"   ||
        ext == ".blend" ||
        ext == ".glTF"  ||
        ext == ".fbx"   ||
        ext == ".obj")
    {
        type = Asset::Type::MODEL;
    } else {
        LOG_ERROR("Unknown file extension:", ext);
        return 0xdeadcell;
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
    case Asset::Type::MODEL: {
        handle = loadModel(path);
    } break;
    default:
        LOG_ERROR("Unknown asset type:", path);
    }

    paths_[path] = handle;
    // LOG_INFO("path:", path);

    hk::event::EventContext context;
    context.u32[0] = handle;
    context.u32[1] = static_cast<u32>(type);
    hk::event::fire(hk::event::EVENT_ASSET_LOADED, context);

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
        // not create -> reload
        hk::bkr::destroy_image(texture->image);

        loader::ImageInfo info = hk::loader::load_image(texture->path);

        ImageDesc desc = {};
        desc.type = ImageType::TEXTURE,
        desc.format = hk::Format::R8G8B8A8_SRGB;
        desc.width = info.width;
        desc.height = info.height;
        desc.channels = info.channels;

        texture->image = hk::bkr::create_image(desc, asset->name);
        hk::bkr::write_image(texture->image, info.pixels);
    } break;
    case Asset::Type::MATERIAL: {
        // TODO: do
        LOG_ERROR("Material hot reload is not yet implemented");
    } break;
    case Asset::Type::MODEL: {
        // TextureAsset *texture = reinterpret_cast<TextureAsset*>(asset);
        // texture->texture = hk::loader::loadImage(texture->path);
        // TODO: do
        LOG_ERROR("Model hot reload is not yet implemented");
    } break;
    }
}

void AssetManager::attachCallback(u32 handle, std::function<void()> callback)
{
    if (callbacks_.size() <= index_) {
        callbacks_.resize(index_ + 1);
    }

    callbacks_.at(getIndex(handle)).push_back(callback);
}

u32 AssetManager::loadTexture(const std::string &path)
{
    TextureAsset *asset = new TextureAsset();
    assets_.push_back(asset);
    asset->handle = index_++;

    asset->name = path.substr(path.find_last_of("/\\") + 1);
    asset->path = path;
    asset->type = Asset::Type::TEXTURE;

    loader::ImageInfo info = hk::loader::load_image(path);

    ImageDesc desc = {};
    desc.type = ImageType::TEXTURE,
    desc.format = hk::Format::R8G8B8A8_SRGB;
    desc.width = info.width;
    desc.height = info.height;
    desc.channels = info.channels;

    asset->image = hk::bkr::create_image(desc, asset->name);
    hk::bkr::write_image(asset->image, info.pixels);

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
    assets_.push_back(asset);
    asset->handle = index_++;

    asset->name = path.substr(path.find_last_of("/\\") + 1);
    asset->path = path;
    asset->type = Asset::Type::SHADER;
    asset->desc = desc;
    asset->code = code;
    asset->createShaderModule();

    return asset->handle;
}

u32 AssetManager::loadModel(const std::string &path)
{
    ModelAsset *asset = new ModelAsset();
    assets_.push_back(asset);
    asset->handle = index_++;

    asset->name = path.substr(path.find_last_of("/\\") + 1);
    asset->path = path;
    asset->type = Asset::Type::MODEL;

    asset->hndlRootMesh = hk::loader::loadModel(path);

    return asset->handle;
}

u32 AssetManager::createMaterial(void *data)
{
    MaterialAsset *asset = reinterpret_cast<MaterialAsset*>(data);
    assets_.push_back(asset);
    asset->handle = index_++;

    asset->type = Asset::Type::MATERIAL;

    createFallbackTextures();

    if (!asset->data.map_handles[Material::BASECOLOR]) {
        asset->data.map_handles[Material::BASECOLOR] = hndl_fallback_color;
    }

    for (u32 i = Material::BASECOLOR + 1; i < Material::MAX_TEXTURE_TYPE; ++i) {
        if (asset->data.map_handles[i]) { continue; }
        asset->data.map_handles[i] = hndl_fallback_noncolor;
    }

    const std::string path = "..\\engine\\assets\\shaders\\";
    asset->data.vertex_shader = hk::assets()->load(path + "Default.vert.hlsl");
    asset->data.pixel_shader = hk::assets()->load(path + "Deferred.frag.hlsl");

    hk::assets()->attachCallback(asset->data.vertex_shader, [&](){
        hk::event::EventContext context;
        context.u32[0] = asset->handle;
        hk::event::fire(event::EVENT_MATERIAL_MODIFIED, context);
    });
    hk::assets()->attachCallback(asset->data.pixel_shader, [&](){
        hk::event::EventContext context;
        context.u32[0] = asset->handle;
        hk::event::fire(event::EVENT_MATERIAL_MODIFIED, context);
    });

    return asset->handle;
}

u32 AssetManager::createMesh(void *data)
{
    MeshAsset *asset = reinterpret_cast<MeshAsset*>(data);
    assets_.push_back(asset);
    asset->handle = index_++;
    asset->type = Asset::Type::MESH;

    for (auto &mesh : asset->children) {
        create(Asset::Type::MESH, mesh);
    }

    return asset->handle;
}

void AssetManager::createFallbackTextures()
{
    if (hndl_fallback_color) { return; }

    // Create fallback textures for color map and non-color maps
    hndl_fallback_color = load("PNG\\Purple\\texture_08.png");

    TextureAsset *asset = new TextureAsset();
    assets_.push_back(asset);
    asset->handle = index_++;

    asset->name = "Fallback Transparent Texture";
    asset->type = Asset::Type::TEXTURE;

    hk::ImageHandle image = hk::bkr::create_image({
        ImageType::TEXTURE,
        hk::Format::R8G8B8A8_SRGB,
        1, 1, 4
    }, asset->name);

    u8 *data = new u8[4];
    data[0] = 0;
    data[1] = 0;
    data[2] = 0;
    data[3] = 0;
    hk::bkr::write_image(image, data);
    delete[] data;

    asset->image = image;

    hndl_fallback_noncolor = asset->handle;
}

}
