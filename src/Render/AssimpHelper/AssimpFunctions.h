//----------------------------------------------------------------------------
//-- AssimpFunctions.h
//-- VlaGan: Useful functions/methods to work with Assimp library
//----------------------------------------------------------------------------
#pragma once
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/matrix4x4.h>
#include <DirectXMath.h>

//-- get bone that affects on mesh
static aiBone* GetAiBone(const aiScene* scene, std::string name) {
    for (unsigned i{}; i < scene->mNumMeshes; ++i) {
        const aiMesh* mesh = scene->mMeshes[i];
        for (unsigned j{}; j < mesh->mNumBones; ++j) {
            aiBone* bone = mesh->mBones[j];
            if (bone->mName.C_Str() == name)
                return bone;
        }
    }
    return nullptr;
}

//-- calc trans from node to parrent
static aiMatrix4x4 calcTrans(const aiNode* node, const aiNode* limit_node) {

    if (!node->mParent || node == limit_node)
        return node->mTransformation;
    else
        return calcTrans(node->mParent, limit_node) * node->mTransformation;
};

//-- convert assimp matrix to dx matrix
inline DirectX::XMMATRIX AssimpToXMMatrix(const aiMatrix4x4& m)
{
    return DirectX::XMMATRIX(
        m.a1, m.b1, m.c1, m.d1,
        m.a2, m.b2, m.c2, m.d2,
        m.a3, m.b3, m.c3, m.d3,
        m.a4, m.b4, m.c4, m.d4
    );
}
