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
    hk::Material *mat = new Material();

    // f32 diffuse[4];
    // material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse);
    // mat->diffuse = hkm::vec4f(diffuse);
    //
    // f32 specular[4];
    // material->Get(AI_MATKEY_COLOR_SPECULAR, specular);
    // mat->specular = hkm::vec4f(specular);
    //
    // f32 shininess;
    // material->Get(AI_MATKEY_SHININESS, shininess);
    // mat->shininess = shininess;

    // Load textures
    u32 hndlTexture = 0;
    for (u32 i = 0; i < material->GetTextureCount(aiTextureType_DIFFUSE); ++i) {
        aiString asspath;
        if (material->GetTexture(aiTextureType_DIFFUSE, i, &asspath) == AI_SUCCESS) {
            hndlTexture = hk::assets()->load(path.substr(0, path.find_last_of("/\\") + 1) + asspath.C_Str());
            mat->diffuse = hk::assets()->getTexture(hndlTexture).texture;
        }
    }

    // aiString name;
    // material->Get(AI_MATKEY_NAME, name);
    // mat->name = name.C_Str();

    return hk::assets()->create(hk::Asset::Type::MATERIAL, mat);
}

u32 loadModel(const std::string &path)
{
    Assimp::Importer importer;

    i32 flags = aiProcess_Triangulate |
                aiProcess_GenBoundingBoxes |
                aiProcess_FlipUVs |
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
            // vertex.tangent   = reinterpret_cast<hkm::vec3f&>(srcMesh->mTangents[v]);
            // vertex.bitangent = reinterpret_cast<hkm::vec3f&>(srcMesh->mBitangents[v]) * -1.f; // Flip V
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
