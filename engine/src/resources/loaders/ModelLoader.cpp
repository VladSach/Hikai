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

Model* loadModel(const std::string &path)
{
    Assimp::Importer importer;

    i32 flags = aiProcess_Triangulate |
                aiProcess_GenBoundingBoxes |
                aiProcess_FlipUVs |
                aiProcess_CalcTangentSpace;

    const aiScene *assimpScene = importer.ReadFile(path, flags);
    ALWAYS_ASSERT(assimpScene, "Failed to load model:", path);

    const u32 numMeshes = assimpScene->mNumMeshes;

    Model *model = new Model();
    model->meshes_.resize(numMeshes);

    for (u32 i = 0; i < numMeshes; ++i) {
        const aiMesh* srcMesh = assimpScene->mMeshes[i];
        Mesh& dstMesh = model->meshes_[i];

        dstMesh.vertices.resize(srcMesh->mNumVertices);
        dstMesh.triangles.resize(srcMesh->mNumFaces);

        for (u32 v = 0; v < srcMesh->mNumVertices; ++v) {
            Vertex& vertex = dstMesh.vertices[v];

            vertex.pos       = reinterpret_cast<hkm::vec3f&>(srcMesh->mVertices[v]);
            vertex.tc        = reinterpret_cast<hkm::vec2f&>(srcMesh->mTextureCoords[0][v]);
            vertex.normal    = reinterpret_cast<hkm::vec3f&>(srcMesh->mNormals[v]);
            // vertex.tangent   = reinterpret_cast<hkm::vec3f&>(srcMesh->mTangents[v]);
            // vertex.bitangent = reinterpret_cast<hkm::vec3f&>(srcMesh->mBitangents[v]) * -1.f; // Flip V
        }

        for (u32 f = 0; f < srcMesh->mNumFaces; ++f) {
            const aiFace& face = srcMesh->mFaces[f];
            ALWAYS_ASSERT(face.mNumIndices == 3); // Triangulated model
            dstMesh.triangles[f] = *reinterpret_cast<hk::Mesh::triangle*>(face.mIndices);
            // indices.push_back(static_cast<u32>(vertices.size() - srcMesh->mNumVertices + face.mIndices[0]));
            // indices.push_back(static_cast<u32>(vertices.size() - srcMesh->mNumVertices + face.mIndices[1]));
            // indices.push_back(static_cast<u32>(vertices.size() - srcMesh->mNumVertices + face.mIndices[2]));
        }
    }

    // Recursively load mesh instances (meshToModel transformation matrices)
    std::function<void(aiNode*)> loadInstances;
    loadInstances = [&loadInstances, &model](aiNode* node)
    {
        const hkm::mat4f nodeToParent = reinterpret_cast<const hkm::mat4f&>(node->mTransformation.Transpose());
        // const hkm::mat4f parentToNode = inverse(nodeToParent);

        // The same node may contain multiple meshes in its space, referring to them by indices
        for (u32 i = 0; i < node->mNumMeshes; i++) {
            u32 meshIndex = node->mMeshes[i];
            model->meshes_[meshIndex].instances.push_back(nodeToParent);
            // model->meshes_[meshIndex].instancesInv.push_back(parentToNode);
        }

        for (u32 i = 0; i < node->mNumChildren; i++) {
            loadInstances(node->mChildren[i]);
        }
    };

    loadInstances(assimpScene->mRootNode);

    u32 hndlTexture = 0;
    hk::vector<u32> diffuseTextureHandles;
    for (u32 i = 0; i < assimpScene->mNumMaterials; i++) {
        auto &material = assimpScene->mMaterials[i];

        u32 diffuseTextureCount = material->GetTextureCount(aiTextureType_DIFFUSE);
        for (u32 j = 0; j < diffuseTextureCount; j++) {
            aiString path;

            if (material->GetTexture(aiTextureType_DIFFUSE, j, &path) == AI_SUCCESS) {
                hndlTexture = hk::assets()->load(path.C_Str());
                diffuseTextureHandles.push_back(hndlTexture);
            }
        }

    }

    if (diffuseTextureHandles.size()) {
        model->diffuse = hk::assets()->getTexture(diffuseTextureHandles.at(0)).texture;
    }

    // LOG_DEBUG("Loaded model:", path);
    // LOG_TRACE("Meshes:", model->meshes_.size());
    // LOG_TRACE("Verticies:", model->vertices.size());
    // LOG_TRACE("Indices:", model->indices.size());
    return model;
}

}
