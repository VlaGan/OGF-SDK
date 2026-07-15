//----------------------------------------------------------------------------
//-- COgfModel.h
//-- Intermediate CPU-side representation of a parsed .ogf file.
//-- COgfLoader fills this structure; CModel::LoadFromOGF then converts it
//-- into CMesh / CSkeleton GPU resources.
//----------------------------------------------------------------------------
#pragma once
#include <string>
#include <vector>
#include <DirectXMath.h>
#include "ogf_format.h"
#include "../../_render_structs.h" // Vertex

//-- one skeleton bone, as read from OGF_S_BONE_NAMES / OGF_S_IKDATA
struct SOgfBoneDef
{
    std::string name;
    std::string parentName;
    int parentIndex = -1; // resolved by COgfLoader once every bone name is known

    //-- bind-pose local transform, X-Ray "XYZ" Euler convention (radians)
    DirectX::XMFLOAT3 rotation{};
    DirectX::XMFLOAT3 position{};
};

//-- one renderable piece of geometry (either the whole model for MT_NORMAL,
//-- or one child of a hierarchical/skeletal model)
struct SOgfMeshDef
{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    std::string textureName; // as stored in the file (no path, no extension)
    std::string shaderName;

    bool isSkinned = false; // true if vertices carry valid boneIDs/weights
};

//-- a fully parsed .ogf file
struct SOgfModel
{
    std::string sourcePath;
    ogf_u8 modelType = 0;
    ogf_u8 formatVersion = 0; // 3 = Shadow of Chornobyl era, 4 = CS/CoP "release" format

    DirectX::XMFLOAT3 bboxMin{};
    DirectX::XMFLOAT3 bboxMax{};

    std::vector<SOgfMeshDef> meshes;
    std::vector<SOgfBoneDef> bones; // empty for static (non-skeletal) models
};
