//----------------------------------------------------------------------------
//-- CModel.h
//-- VlaGan: General class for 3d models (loading use Assimp library) .header
//----------------------------------------------------------------------------
#pragma once

#include <d3d11.h>
#include <string>
#include <vector>
#include <unordered_map>
#include "CMesh.h"
#include "CMotion.h"
#include "CSkeleton.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "../Templates/ConstantBuffer.h"


class CModel {
public:
    CModel() = default;
    ~CModel();

    bool LoadFromFile(ID3D11Device* device, const std::string& path);

    //-- native .ogf loading (no Assimp): parses the file directly via
    //-- COgfLoader and builds CMesh/CSkeleton from it. Bind-pose skinning
    //-- works out of the box; motion (.omf) playback is not implemented yet,
    //-- so `scene` stays null and TraverseHierarchy()/Update() simply keeps
    //-- rendering the bind pose via UpdateSkeleton().
    bool LoadFromOGF(ID3D11Device* device, const std::string& path);

    void Render(ID3D11DeviceContext* context, bool transparent = false);
    void RenderShadowMap(ID3D11DeviceContext* context);

    void UpdateBones(ID3D11DeviceContext* context);

    void Release();

    void Update(float dt);

    //-- transform methods
    void SetPosition(float x, float y, float z) { m_Position = { x, y, z }; }
    void SetRotation(float pitch, float yaw, float roll) { m_Rotation = { pitch, yaw, roll }; }
    void SetScale(float x, float y, float z) { m_Scale = { x, y, z }; }

    DirectX::XMFLOAT3 Position() { return m_Position; }
    DirectX::XMFLOAT3 Rotation() { return m_Rotation; }
    DirectX::XMFLOAT3 Scale() { return m_Scale; }

    DirectX::XMMATRIX XFORM() { return m_trans; }

    void UpdateTransform();

    void SetShaderOnMesh();
public:

    //-- Assimp loading scene
    Assimp::Importer importer;
    const aiScene* scene{};

    //-- Model Meshes
    std::vector<CMesh> m_Meshes;

    //-- General transformations
    DirectX::XMFLOAT3 m_Position{};
    DirectX::XMFLOAT3 m_Rotation{}; // radians
    DirectX::XMFLOAT3 m_Scale{ 1.f, 1.f, 1.f };

    DirectX::XMMATRIX m_trans{};

    //-- Model Data
    std::string m_modelPath{};
    std::string m_modelName{};

    //-- Skeleton data
    CSkeleton m_Skeleton;

    //-- Motions and model animating
    //-- get root bone name
    std::string rootName() { return m_Skeleton.GetRootBone()->m_sBoneName; }

    //-- key position, rotation, scale for all bones in 1 motion 
    // (need CAnimation class cuz assimp so wierd)
    //std::vector<BoneAnim> m_BoneAnimations;

    void SetMotion(CMotion* mt);
    CMotion* GetMotion(std::string name);

    std::vector<CMotion> m_vMotions;
    CMotion* m_pCurrentMotion{};

    void TraverseHierarchy(float animationTime, const aiNode* node, const DirectX::XMMATRIX& parentTransform);
    void UpdateSkeleton(CBoneInstance* bone, const DirectX::XMMATRIX& parentTransform);

    //-- Current motion parameters (maybe i will take it from motion data, thats just for debug)
    float m_AnimationDuration = 358.f;
    float m_TicksPerSecond = 0.0f;
    float m_CurrentTime{}; //seconds

    CConstantBuffer m_BoneBuffer;
};
