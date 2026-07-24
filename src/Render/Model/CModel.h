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
#include "OGF/COgfModel.h"

class CModel;

//-- attach data
struct CAttachData {
    DirectX::XMFLOAT3 m_attachPos{};
    DirectX::XMFLOAT3 m_attachRot{};
    DirectX::XMFLOAT3 m_attachScale{ 1.f, 1.f, 1.f };

    std::string m_ParentBone{};

    //-- specific bone for attach to parent bone (default - root)
    std::string m_ChildBone{};

    CModel* m_pParent{};
    DirectX::XMMATRIX m_ParentTransform{ DirectX::XMMatrixIdentity() };

    bool valid() { return m_pParent != nullptr; }
    void zero() {
        m_attachPos = { 0.f, 0.f, 0.f };
        m_attachRot = { 0.f, 0.f, 0.f };
        m_attachScale = { 1.f, 1.f, 1.f };
        m_pParent = nullptr;
        m_ParentBone.clear();
        m_ChildBone.clear();
    }
};


class CModel {
public:
    CModel() = default;
    ~CModel();

    bool LoadFromFile(ID3D11Device* device, const std::string& path);

    //-- native .ogf loading (no Assimp): parses the file directly via
    //-- COgfLoader and builds CMesh/CSkeleton from it, and also resolves+loads
    //-- any referenced .omf motion files into m_vMotions. `scene` stays null;
    //-- Update() uses TraverseSkeleton() instead of TraverseHierarchy() for
    //-- playback, and UpdateSkeleton() (bind pose) whenever no motion is set.
    bool LoadFromOGF(ID3D11Device* device, const std::string& path);

    void Render(ID3D11DeviceContext* context, bool transparent = false);
    void RenderShadowMap(ID3D11DeviceContext* context);
    void RenderOutline(ID3D11DeviceContext* context, float thickness, DirectX::XMFLOAT3 color);

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
    void UpdateTransformAttached(float dt, float animTime);

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

    //-- OGF_S_DESC / OGF_S_LODS metadata - only populated for natively (.ogf)
    //-- loaded models (LoadFromOGF), empty/default for the Assimp path.
    SOgfDescription m_Description;
    std::string m_LodPath;

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

    //-- same as TraverseHierarchy(), but walks CSkeleton's own CBoneInstance
    //-- tree instead of an Assimp aiNode tree - used for natively (.ogf)
    //-- loaded models, which have no aiScene to traverse.
    void TraverseSkeleton(float animationTime, CBoneInstance* bone, const DirectX::XMMATRIX& parentTransform);

    void UpdateSkeleton(CBoneInstance* bone, const DirectX::XMMATRIX& parentTransform);

    //-- Current motion parameters (maybe i will take it from motion data, thats just for debug)
    float m_AnimationDuration = 358.f;
    float m_TicksPerSecond = 0.0f;
    float m_CurrentTime{}; //seconds

    CConstantBuffer m_BoneBuffer;

    //-- attach data
    CAttachData m_AttachData;

    //-- for text exporting, maybe i will refactor CModel
    SOgfModel m_OgfSource;
};
