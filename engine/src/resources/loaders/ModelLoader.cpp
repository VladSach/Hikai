#include "ModelLoader.h"

#include "utils/containers/hkvector.h"
#include "resources/AssetManager.h"

#include "vendor/assimp/Importer.hpp"
#include "vendor/assimp/scene.h"
#include "vendor/assimp/postprocess.h"

// TODO: replace with Hikai implementation
#include <unordered_map>
#include <functional>

namespace hk::loader {

u32 loadMaterial(const aiMaterial* material, const std::string &path)
{
    hk::MaterialAsset *asset = new MaterialAsset();

    // path without filename
    const std::string path_ = path.substr(0, path.find_last_of("/\\") + 1);

    aiString name;
    material->Get(AI_MATKEY_NAME, name);
    asset->name = name.C_Str();
    asset->path = path;

    hk::Material &mat = asset->data;

    // Get constants
    material->Get(AI_MATKEY_COLOR_DIFFUSE,  mat.constants.color);
    material->Get(AI_MATKEY_COLOR_SPECULAR, mat.constants.specular);
    material->Get(AI_MATKEY_COLOR_AMBIENT,  mat.constants.ambient);
    // material->Get(AI_MATKEY_COLOR_EMISSIVE, mat.constants.emissive);
    // material->Get(AI_MATKEY_COLOR_TRANSPARENT, mat.constants.emissive);
    // material->Get(AI_MATKEY_COLOR_REFLECTIVE,  mat.constants.emissive);

    material->Get(AI_MATKEY_TWOSIDED,       mat.twosided);
    // material->Get(AI_MATKEY_BLEND_FUNC,       mat.constants.twosided);

    material->Get(AI_MATKEY_OPACITY,        mat.constants.opacity);

    // material->Get(AI_MATKEY_SHININESS,    mat.constants.shininess);
    // material->Get(AI_MATKEY_REFLECTIVITY, mat.constants.reflectivity);
    material->Get(AI_MATKEY_METALLIC_FACTOR,  mat.constants.metalness);
    material->Get(AI_MATKEY_ROUGHNESS_FACTOR, mat.constants.roughness);

    // Load textures
    aiString asspath;
    for (u32 i = 0; i < material->GetTextureCount(aiTextureType_DIFFUSE); ++i) {
        if (material->GetTexture(aiTextureType_DIFFUSE, i, &asspath) == AI_SUCCESS) {
            mat.map_handles[Material::BASECOLOR] = hk::assets()->load(path_ + asspath.C_Str());
        }
    }

    for (u32 i = 0; i < material->GetTextureCount(aiTextureType_EMISSIVE); ++i) {
        if (material->GetTexture(aiTextureType_EMISSIVE, i, &asspath) == AI_SUCCESS) {
            mat.map_handles[Material::EMISSIVE] = hk::assets()->load(path_ + asspath.C_Str());
        }
    }

    for (u32 i = 0; i < material->GetTextureCount(aiTextureType_NORMALS); ++i) {
        if (material->GetTexture(aiTextureType_NORMALS, i, &asspath) == AI_SUCCESS) {
            mat.map_handles[Material::NORMAL] = hk::assets()->load(path_ + asspath.C_Str());
        }
    }

    for (u32 i = 0; i < material->GetTextureCount(aiTextureType_SHININESS); ++i) {
        if (material->GetTexture(aiTextureType_SHININESS, i, &asspath) == AI_SUCCESS) {
            mat.map_handles[Material::ROUGHNESS] = hk::assets()->load(path_ + asspath.C_Str());
        }
    }

    for (u32 i = 0; i < material->GetTextureCount(aiTextureType_LIGHTMAP); ++i) {
        if (material->GetTexture(aiTextureType_LIGHTMAP, i, &asspath) == AI_SUCCESS) {
            mat.map_handles[Material::AMBIENT_OCCLUSION] = hk::assets()->load(path_ + asspath.C_Str());
        }
    }

    // PBR Materials
    for (u32 i = 0; i < material->GetTextureCount(aiTextureType_BASE_COLOR); ++i) {
        if (material->GetTexture(aiTextureType_BASE_COLOR, i, &asspath) == AI_SUCCESS) {
            mat.map_handles[Material::BASECOLOR] = hk::assets()->load(path_ + asspath.C_Str());
        }
    }

    for (u32 i = 0; i < material->GetTextureCount(aiTextureType_NORMAL_CAMERA); ++i) {
        if (material->GetTexture(aiTextureType_NORMAL_CAMERA, i, &asspath) == AI_SUCCESS) {
            mat.map_handles[Material::NORMAL] = hk::assets()->load(path_ + asspath.C_Str());
        }
    }

    for (u32 i = 0; i < material->GetTextureCount(aiTextureType_EMISSION_COLOR); ++i) {
        if (material->GetTexture(aiTextureType_EMISSION_COLOR, i, &asspath) == AI_SUCCESS) {
            mat.map_handles[Material::EMISSIVE] = hk::assets()->load(path_ + asspath.C_Str());
        }
    }

    for (u32 i = 0; i < material->GetTextureCount(aiTextureType_METALNESS); ++i) {
        if (material->GetTexture(aiTextureType_METALNESS, i, &asspath) == AI_SUCCESS) {
            mat.map_handles[Material::METALNESS] = hk::assets()->load(path_ + asspath.C_Str());
        }
    }

    for (u32 i = 0; i < material->GetTextureCount(aiTextureType_DIFFUSE_ROUGHNESS); ++i) {
        if (material->GetTexture(aiTextureType_DIFFUSE_ROUGHNESS, i, &asspath) == AI_SUCCESS) {
            mat.map_handles[Material::ROUGHNESS] = hk::assets()->load(path_ + asspath.C_Str());
        }
    }

    for (u32 i = 0; i < material->GetTextureCount(aiTextureType_AMBIENT_OCCLUSION); ++i) {
        if (material->GetTexture(aiTextureType_AMBIENT_OCCLUSION, i, &asspath) == AI_SUCCESS) {
            mat.map_handles[Material::AMBIENT_OCCLUSION] = hk::assets()->load(path_ + asspath.C_Str());
        }
    }

    return hk::assets()->create(hk::Asset::Type::MATERIAL, asset);
}

u32 loadModel(const std::string &path)
{
    Assimp::Importer importer;

    i32 flags = aiProcess_Triangulate |
                aiProcess_GenBoundingBoxes |
                // aiProcess_FlipUVs |
                aiProcess_ConvertToLeftHanded |
                aiProcess_CalcTangentSpace;

    const aiScene *assimpScene = importer.ReadFile(path, flags);
    ALWAYS_ASSERT(assimpScene, "Failed to load model:", path);

    const u32 numMeshes = assimpScene->mNumMeshes;
    const u32 numMaterials = assimpScene->mNumMaterials;

    hk::vector<Mesh> meshes;
    hk::vector<u32> materials;

    meshes.resize(numMeshes);
    materials.reserve(numMaterials);

    for (u32 i = 0; i < numMaterials; ++i) {
        const aiMaterial* material = assimpScene->mMaterials[i];
        u32 hndlMaterial = loadMaterial(material, path);
        materials.push_back(hndlMaterial);
    }

    for (u32 i = 0; i < numMeshes; ++i) {
        const aiMesh* srcMesh = assimpScene->mMeshes[i];
        Mesh &dstMesh = meshes[i];

        dstMesh.vertices.resize(srcMesh->mNumVertices);

        for (u32 v = 0; v < srcMesh->mNumVertices; ++v) {
            Vertex &vertex = dstMesh.vertices[v];

            vertex.pos       = reinterpret_cast<hkm::vec3f&>(srcMesh->mVertices[v]);
            vertex.tc        = reinterpret_cast<hkm::vec2f&>(srcMesh->mTextureCoords[0][v]);
            vertex.normal    = reinterpret_cast<hkm::vec3f&>(srcMesh->mNormals[v]);
            vertex.tangent   = reinterpret_cast<hkm::vec3f&>(srcMesh->mTangents[v]);
            vertex.bitangent = reinterpret_cast<hkm::vec3f&>(srcMesh->mBitangents[v]) * -1.f; // Flip V
        }

        for (u32 f = 0; f < srcMesh->mNumFaces; ++f) {
            const aiFace& face = srcMesh->mFaces[f];
            ALWAYS_ASSERT(face.mNumIndices == 3); // Triangulated model
            dstMesh.indices.push_back(static_cast<u32>(face.mIndices[0]));
            dstMesh.indices.push_back(static_cast<u32>(face.mIndices[1]));
            dstMesh.indices.push_back(static_cast<u32>(face.mIndices[2]));
        }
    }

    // Recursively load mesh instances and material
    std::function<void(aiNode*, MeshAsset *parent)> loadInstances;
    loadInstances = [&](aiNode* node, MeshAsset *parent = nullptr)
    {
        MeshAsset *currentMesh;

        // Skip nodes without mesh
        // TEST: this might result in skipping transforms, should test this
        if (node->mNumMeshes > 0) {
            currentMesh = new MeshAsset();
            currentMesh->name = node->mName.C_Str();

            if (parent) {
                parent->children.push_back(currentMesh);
            }

            const hkm::mat4f nodeToParent = reinterpret_cast<const hkm::mat4f&>(node->mTransformation.Transpose());
            const hkm::mat4f parentToNode = inverse(nodeToParent);

            // The same node may contain multiple meshes in its space, referring to them by indices
            for (u32 i = 0; i < node->mNumMeshes; i++) {
                u32 meshIndex = node->mMeshes[i];

                // Load Instances
                currentMesh->mesh = meshes[meshIndex];
                currentMesh->instances.push_back(nodeToParent);
                currentMesh->instancesInv.push_back(parentToNode);

                // Load Materials
                const aiMesh* mesh = assimpScene->mMeshes[meshIndex];
                currentMesh->hndlTextures.push_back(materials[mesh->mMaterialIndex]);
            }
        } else {
            currentMesh = parent;
        }

        for (u32 i = 0; i < node->mNumChildren; i++) {
            loadInstances(node->mChildren[i], currentMesh);
        }
    };

    MeshAsset *root = new MeshAsset();
    root->name = "Meshes";
    loadInstances(assimpScene->mRootNode, root);

    return hk::assets()->create(Asset::Type::MESH, root);
}

}
