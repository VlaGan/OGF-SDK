//----------------------------------------------------------------------------
//-- CModel.cpp
//-- VlaGan: General class for 3d models (loading use Assimp library)
//----------------------------------------------------------------------------
#include "CModel.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/matrix4x4.h>
#include <functional>

#include "../_render_structs.h"
#include "../CRenderer.h"
#include "CMotion.h"
#include "OGF/COgfLoader.h"

#include "../../Core/CSettings.h"


//-- Constructor / Destructor
CModel::~CModel() {
    Release();
}

//-- Release Model Data
void CModel::Release() {
    //-- Release Meshes
    //for (auto& mesh : m_Meshes)
    //    mesh.Release();
}

//----------------------------------------------------------------------------
//-- VlaGan: Model Loading
//----------------------------------------------------------------------------
//-- adding bones that affects on mesh (only MAX_BONE_INFLUENCE bones can affect)
void AddBoneData(Vertex& vertex, int boneID, float weight) {
    for (int i = 0; i < MAX_BONE_INFLUENCE; i++) {
        if (vertex.weights[i] == 0.0f) {
            vertex.boneIDs[i] = boneID;
            vertex.weights[i] = weight;
            return;
        }
    }
}

void clamp(float& v, float min, float max) {
    if (v < min)
        v = min;
    if (v > max)
        v = max;
}

bool CModel::LoadFromFile(ID3D11Device* device, const std::string& path)
{
    try {
        scene = importer.ReadFile(path,
            aiProcess_Triangulate | aiProcess_GenNormals | 
            aiProcess_JoinIdenticalVertices | aiProcess_PopulateArmatureData | aiProcess_MakeLeftHanded);
    }
    catch (...) {
        MessageBox(nullptr, L"CModel::LoadFromFile:[!scene || !scene->HasMeshes()]", L"Error", MB_OK);
    }
    if (!scene || !scene->HasMeshes()) {
        MessageBox(nullptr, L"CModel::LoadFromFile:[!scene || !scene->HasMeshes()]", L"Error", MB_OK);
        return false;
    }

    //-- get model path and name
    m_modelPath = path;
    m_modelName = scene->GetShortFilename(m_modelPath.c_str());

    LogMsg("!Loading model [%s].", m_modelName.c_str());

    // loading textures
    //std::unordered_map<unsigned, std::string> meshTextureMap;
    for (unsigned int i = 0; i < scene->mNumMaterials; ++i) {
        aiMaterial* material = scene->mMaterials[i];

        LogMsg("~Material [%u]:", i);

        // ²ì'ÿ ìàòåð³àëó
        aiString name;
        if (material->Get(AI_MATKEY_NAME, name) == AI_SUCCESS)
            LogMsg("    Name: %s", name.C_Str());

        // Òèï øåéäèíãó
        int shadingModel = 0;
        if (material->Get(AI_MATKEY_SHADING_MODEL, shadingModel) == AI_SUCCESS)
            LogMsg("    Shading model: %d", shadingModel); // Íàïðèêëàä, aiShadingMode_Phong = 1

        // Ïðîçîð³ñòü (Opacity)
        float opacity = 1.0f;
        if (material->Get(AI_MATKEY_OPACITY, opacity) == AI_SUCCESS)
            LogMsg("    Opacity: %.2f", opacity);

        // Àëüôà ðåæèì
        int alphaMode = 0;
        if (material->Get(AI_MATKEY_BLEND_FUNC, alphaMode) == AI_SUCCESS)
            LogMsg("    Alpha Mode (Blend Func): %d", alphaMode);

        // Diffuse Textures
        unsigned int texCount = material->GetTextureCount(aiTextureType_DIFFUSE);
        LogMsg("    Diffuse Textures: %u", texCount);
        for (unsigned int t = 0; t < texCount; ++t) {
            aiString texturePath;
            if (material->GetTexture(aiTextureType_DIFFUSE, t, &texturePath) == AI_SUCCESS) {
                LogMsg("        [%u] Path: %s", t, texturePath.C_Str());
            }
        }

        // Alpha Mask Textures (ÿêùî º)
        unsigned int alphaTexCount = material->GetTextureCount(aiTextureType_OPACITY);
        if (alphaTexCount > 0) {
            LogMsg("    Opacity (Alpha) Textures: %u", alphaTexCount);
            for (unsigned int t = 0; t < alphaTexCount; ++t) {
                aiString alphaPath;
                if (material->GetTexture(aiTextureType_OPACITY, t, &alphaPath) == AI_SUCCESS) {
                    LogMsg("        [%u] AlphaTex: %s", t, alphaPath.C_Str());
                }
            }
        }

        LogMsg(""); // Empty line for spacing
    }

    //-- Load Skeleton
    //-- if skeleton have no motions, maybe need to take local bones postions
    //-- just for correct drawing without visual bugs
    m_Skeleton.Load(scene);

    //-- loading meshes
    for (unsigned i{}; i < scene->mNumMeshes; ++i) {
        CMesh meshObj;
        aiMaterial* mat = scene->mMaterials[scene->mMeshes[i]->mMaterialIndex];
        const aiMesh* mesh = scene->mMeshes[i];

        aiString texPath;
        std::string tmp, texture;
        if (mat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS)
            tmp = texPath.C_Str();
        
        for (size_t i = tmp.size() - 1; i >= 0; i--)
            if (tmp[i] == '\\')
                break;
            else
                texture.insert(texture.begin(), tmp[i]);

        //LogMsg("~Texture for mesh %s", texture.c_str());
        texture = "appdata/textures/" + m_modelName + "/" + texture;

        if (meshObj.Init(device, scene->mMeshes[i], texture)) {

            //-- loading bone weights for vertices
            for (unsigned int b = 0; b < mesh->mNumBones; ++b) {
                aiBone* bone = mesh->mBones[b];
                std::string boneName(bone->mName.C_Str());

                for (unsigned int w = 0; w < bone->mNumWeights; ++w) {
                    int vertexID = bone->mWeights[w].mVertexId;
                    float weight = bone->mWeights[w].mWeight;
                    clamp(weight, 0.f, 1.f);

                    AddBoneData(meshObj.m_vVertices[vertexID], m_Skeleton.GetBoneInstance(boneName)->m_bBoneID, weight);
                }
                
            }
            //-- cuz i fill vb in CMesh::Init (pos, norm, uv) and here bones and weights
            meshObj.UpdateVertexBuffer(device);
            //-- add mesh to vertex
            m_Meshes.push_back(std::move(meshObj));
        }

        else {
            LogMsg("~CModel::LoadFromFile: cant load mesh for model [%s]", m_modelName.c_str());
            return false;
        }
    }

    //-- TODO: need it for every motion (+- now need to best code visual)
    for (unsigned int a = 0; a < scene->mNumAnimations; ++a) //-- just test (last motion)
    {
        aiAnimation* anim = scene->mAnimations[a];
        LogMsg("- Animation name: %s | Duration: %.2f | Ticks/sec: %.2f",
            anim->mName.C_Str(), anim->mDuration, anim->mTicksPerSecond);

        //-- full motion data
        CMotion motion(anim->mName.C_Str(), 
            (float)anim->mDuration, (float)anim->mTicksPerSecond);

        for (unsigned int c = 0; c < anim->mNumChannels; ++c)
        {
            aiNodeAnim* channel = anim->mChannels[c];

            BoneMotionData boneAnim{};
            boneAnim.name = channel->mNodeName.C_Str();

            for (unsigned int i = 0; i < channel->mNumPositionKeys; ++i) {
                auto& key = channel->mPositionKeys[i];
                boneAnim.positions.push_back({ {key.mValue.x, key.mValue.y, key.mValue.z}, (float)key.mTime });
            }

            for (unsigned int i = 0; i < channel->mNumRotationKeys; ++i) {
                auto& key = channel->mRotationKeys[i];
                boneAnim.rotations.push_back({
                    {key.mValue.x, key.mValue.y, key.mValue.z, key.mValue.w},
                    (float)key.mTime });
            }

            for (unsigned int i = 0; i < channel->mNumScalingKeys; ++i) {
                auto& key = channel->mScalingKeys[i];
                boneAnim.scales.push_back({ {key.mValue.x, key.mValue.y, key.mValue.z}, (float)key.mTime });
            }

            //-- collect bone motion
            motion.AddBoneMtData(boneAnim);
        }
        //-- collect full motion
        m_vMotions.push_back(motion);
    }

    //-- setup default motion
    if(m_vMotions.size())
    SetMotion(&m_vMotions[0]);

    //-- bones buffer
    m_BoneBuffer.Create(device, sizeof(DirectX::XMMATRIX) * MAX_BONES);

    return true;
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//-- VlaGan: Native .ogf Model Loading (no Assimp)
//----------------------------------------------------------------------------
bool CModel::LoadFromOGF(ID3D11Device* device, const std::string& path)
{
    SOgfModel ogf;
    if (!COgfLoader::Load(path, ogf)) {
        MessageBox(nullptr, L"CModel::LoadFromOGF: failed to parse .ogf file", L"Error", MB_OK);
        return false;
    }

    if (ogf.meshes.empty()) {
        LogMsg(eLogLevel::ERR, "!CModel::LoadFromOGF: model [%s] has no supported geometry", path.c_str());
        return false;
    }

    m_OgfSource = ogf;

    //-- native path doesn't use Assimp: `scene` stays null. Update() detects
    //-- this and uses TraverseSkeleton() (walking CSkeleton's own tree)
    //-- instead of TraverseHierarchy() (Assimp's aiNode tree) for motion
    //-- playback, and UpdateSkeleton() (bind pose) whenever no motion is set.
    scene = nullptr;

    m_modelPath = path;
    size_t slashPos = path.find_last_of("/\\");
    m_modelName = (slashPos == std::string::npos) ? path : path.substr(slashPos + 1);
    size_t dotPos = m_modelName.find_last_of('.');
    if (dotPos != std::string::npos)
        m_modelName = m_modelName.substr(0, dotPos);

    LogMsg("!Loading model [%s] (native .ogf).", m_modelName.c_str());

    m_Description = ogf.description;
    m_LodPath = ogf.lodPath;

    if (!ogf.bones.empty())
        m_Skeleton.LoadFromOGF(ogf.bones);

    //-- motions live in separate .omf files, referenced by name from the
    //-- .ogf itself (OGF_S_MOTION_REFS/2) - resolve and decode them now
    if (!ogf.motionRefs.empty()) {
        CSettings& settings = CSettings::Get();
        if (!settings.GamedataPath().empty())
            COgfLoader::LoadMotions(settings.GetMeshesPath(), ogf);
        else
            COgfLoader::LoadMotions(path, ogf);
    }

    m_vMotions.clear();
    m_pCurrentMotion = nullptr;
    if (!ogf.motions.empty()) {
        m_vMotions = std::move(ogf.motions);
        //SetMotion(&m_vMotions[0]);
    }

    m_Meshes.clear();
    m_Meshes.reserve(ogf.meshes.size());

    CSettings& settings = CSettings::Get();
    for (auto& src : ogf.meshes) {
        CMesh meshObj;

        //-- OGF texture names carry no path/extension - textures are still
        //-- expected as pre-converted PNGs under appdata/textures/<model>/,
        //-- same convention LoadFromFile() uses for the Assimp path.
        std::string baseName = src.textureName;
        size_t bs = baseName.find_last_of("/\\");
        if (bs != std::string::npos)
            baseName = baseName.substr(bs + 1);
        std::string texture = settings.GetTexturePath(src.textureName + ".dds");

        if (!meshObj.InitFromRaw(device, std::move(src.vertices), src.indices, texture, src.shaderName)) {
            LogMsg(eLogLevel::ERR, "~CModel::LoadFromOGF: cant load mesh for model [%s]", m_modelName.c_str());
            continue;
        }

        m_Meshes.push_back(std::move(meshObj));
    }

    if (m_Meshes.empty())
        return false;

    //-- bones buffer (bound even for static meshes, matches LoadFromFile())
    m_BoneBuffer.Create(device, sizeof(DirectX::XMMATRIX) * MAX_BONES);

    return true;
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//-- VlaGan: General model update
//-- here updates model transformation and etc.
//----------------------------------------------------------------------------

//-- settuping motion on visual
void CModel::SetMotion(CMotion* mt) {
    if (!mt)
        return;

    m_pCurrentMotion = mt;
    m_AnimationDuration = mt->Duration();
    m_TicksPerSecond = mt->TPS();
    m_CurrentTime = 0.f;
}

//-- getting motion 
CMotion* CModel::GetMotion(std::string name) {
    for (auto& mt : m_vMotions)
        if (mt.GetName() == name)
            return &mt;
    return nullptr;
}

//-- for animating interpolates bone position
DirectX::XMMATRIX InterpolatePosition(const BoneMotionData& boneAnim, float time)
{
    if (!boneAnim.positions.size())
        return DirectX::XMMatrixIdentity();

    if (boneAnim.positions.size() == 1)
        return DirectX::XMMatrixTranslationFromVector(XMLoadFloat3(&boneAnim.positions[0].position));

    for (size_t i = 0; i < boneAnim.positions.size() - 1; ++i)
    {
        if (time < boneAnim.positions[i + 1].timeStamp)
        {
            float dt = boneAnim.positions[i + 1].timeStamp - boneAnim.positions[i].timeStamp;
            float factor = (time - boneAnim.positions[i].timeStamp) / dt;
            auto start = XMLoadFloat3(&boneAnim.positions[i].position);
            auto end = XMLoadFloat3(&boneAnim.positions[i + 1].position);
            auto pos = DirectX::XMVectorLerp(start, end, factor);
            return DirectX::XMMatrixTranslationFromVector(pos);
        }
    }
    
    return DirectX::XMMatrixTranslationFromVector(XMLoadFloat3(&boneAnim.positions.back().position));
}

//-- for animating interpolates bone rotation
DirectX::XMMATRIX InterpolateRotation(const BoneMotionData& boneAnim, float time)
{
    if (!boneAnim.rotations.size())
        return DirectX::XMMatrixIdentity();

    if (boneAnim.rotations.size() == 1)
        return DirectX::XMMatrixRotationQuaternion(DirectX::XMQuaternionNormalize(XMLoadFloat4(&boneAnim.rotations[0].rotation)));

    for (size_t i = 0; i < boneAnim.rotations.size() - 1; ++i)
    {
        if (time < boneAnim.rotations[i + 1].timeStamp)
        {
            float dt = boneAnim.rotations[i + 1].timeStamp - boneAnim.rotations[i].timeStamp;
            float factor = (time - boneAnim.rotations[i].timeStamp) / dt;
            auto start = DirectX::XMQuaternionNormalize(XMLoadFloat4(&boneAnim.rotations[i].rotation));
            auto end = DirectX::XMQuaternionNormalize(XMLoadFloat4(&boneAnim.rotations[i + 1].rotation));
            auto rot = DirectX::XMQuaternionSlerp(start, end, factor);
            return DirectX::XMMatrixRotationQuaternion(rot);
        }
    }

    return DirectX::XMMatrixRotationQuaternion(
        DirectX::XMQuaternionNormalize(XMLoadFloat4(&boneAnim.rotations.back().rotation)));
}

//-- for animating interpolates bone scale
DirectX::XMMATRIX InterpolateScaling(const BoneMotionData& boneAnim, float time)
{
    if (!boneAnim.scales.size())
        return DirectX::XMMatrixIdentity();

    if (boneAnim.scales.size() == 1)
        return DirectX::XMMatrixScalingFromVector(XMLoadFloat3(&boneAnim.scales[0].scale));

    for (size_t i = 0; i < boneAnim.scales.size() - 1; ++i)
    {
        if (time < boneAnim.scales[i + 1].timeStamp)
        {
            float dt = boneAnim.scales[i + 1].timeStamp - boneAnim.scales[i].timeStamp;
            float factor = (time - boneAnim.scales[i].timeStamp) / dt;
            auto start = XMLoadFloat3(&boneAnim.scales[i].scale);
            auto end = XMLoadFloat3(&boneAnim.scales[i + 1].scale);
            auto scl = DirectX::XMVectorLerp(start, end, factor);
            return DirectX::XMMatrixScalingFromVector(scl);
        }
    }

    return DirectX::XMMatrixScalingFromVector(XMLoadFloat3(&boneAnim.scales.back().scale));
}

//extern DirectX::XMFLOAT3 head_ajust;
//-- recursive update all bone matrices transform from root
void CModel::TraverseHierarchy(float animationTime, const aiNode* node, const DirectX::XMMATRIX& parentTransform)
{
    //-- get motion data for current bone
    std::string nodeName(node->mName.C_Str());
    const BoneMotionData* boneAnim = m_pCurrentMotion->GetBoneMotion(nodeName);

    DirectX::XMMATRIX nodeTransform = DirectX::XMMatrixIdentity();

    //-- trash moments
    if (boneAnim)
    {
        auto T = InterpolatePosition(*boneAnim, animationTime);
        auto R = InterpolateRotation(*boneAnim, animationTime);
        auto S = InterpolateScaling(*boneAnim, animationTime);
        nodeTransform = R * T * S; //S * R * T;
    }
    else
    {
        auto pBone = m_Skeleton.GetBoneInstance(node->mName.C_Str());

        nodeTransform = pBone->isIK ? pBone->mLocalTransformIK: pBone->mLocalTransform; //AssimpToXMMatrix(node->mTransformation);
    }

    //-- tweaky skinning test (remove here to collect this by another bones)
    DirectX::XMMATRIX offs = DirectX::XMMatrixIdentity();
    //if (nodeName == "J_Bip_L_LowerArm") {
    //DirectX::XMMATRIX translateUp = DirectX::XMMatrixTranslation(head_ajust.x, head_ajust.y, head_ajust.z);
    //   offs *= translateUp;
    //}
    ////////////////
    DirectX::XMMATRIX globalTransform = nodeTransform * parentTransform * offs;

    if (auto pBone = m_Skeleton.GetBoneInstance(nodeName); pBone) {
        pBone->mGlobalTransform = globalTransform;
        //pBone->mOffsetTransform = DirectX::XMMatrixInverse(nullptr, pBone->mGlobalTransform);
        pBone->mRenderTransform = pBone->mOffsetTransform * globalTransform;
        
        DirectX::XMStoreFloat3(&pBone->m_vWorldPosition, pBone->mGlobalTransform.r[3]);
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        TraverseHierarchy(animationTime, node->mChildren[i], globalTransform);
    }
}

//-- same as TraverseHierarchy() above, but for natively (.ogf) loaded models:
//-- walks CSkeleton's own CBoneInstance tree instead of an Assimp aiNode tree
//-- (there is no aiScene for these models, so no aiNode tree to walk).
void CModel::TraverseSkeleton(float animationTime, CBoneInstance* bone, const DirectX::XMMATRIX& parentTransform)
{
    if (!bone)
        return;

    const BoneMotionData* boneAnim = m_pCurrentMotion->GetBoneMotion(bone->m_sBoneName);

    DirectX::XMMATRIX nodeTransform;
    if (boneAnim)
    {
        auto T = InterpolatePosition(*boneAnim, animationTime);
        //-- X-Ray's Fquaternion -> matrix convention (xrCore/vector.h,
        //-- _matrix::rotation(const _quaternion&)) is the TRANSPOSE of
        //-- DirectXMath's XMMatrixRotationQuaternion (verified numerically -
        //-- every off-diagonal sign is flipped, matching a conjugated
        //-- quaternion / inverse rotation). Without this, animated poses
        //-- rotate the "wrong way", which compounds down the bone hierarchy
        //-- into the model looking turned inside-out.
        auto R = DirectX::XMMatrixTranspose(InterpolateRotation(*boneAnim, animationTime));
        auto S = InterpolateScaling(*boneAnim, animationTime);
        nodeTransform = R * T * S;
    }
    else
    {
        nodeTransform = bone->isIK ? bone->mLocalTransformIK : bone->mLocalTransform;
    }

    DirectX::XMMATRIX globalTransform = nodeTransform * parentTransform;

    bone->mGlobalTransform = globalTransform;
    bone->mRenderTransform = bone->mOffsetTransform * globalTransform;
    DirectX::XMStoreFloat3(&bone->m_vWorldPosition, bone->mGlobalTransform.r[3]);

    for (auto* child : bone->m_vChilds)
        TraverseSkeleton(animationTime, child, globalTransform);
}

//-- general model transform in 3d space (not skeleton, just visual)
void CModel::UpdateTransform() {
    DirectX::XMMATRIX scaleMat = DirectX::XMMatrixScaling(m_Scale.x, m_Scale.y, m_Scale.z);
    DirectX::XMMATRIX rotMat =
        DirectX::XMMatrixRotationX(m_Rotation.x) *
        DirectX::XMMatrixRotationY(m_Rotation.y) *
        DirectX::XMMatrixRotationZ(m_Rotation.z);
    DirectX::XMMATRIX transMat = DirectX::XMMatrixTranslation(m_Position.x, m_Position.y, m_Position.z);
    m_trans = scaleMat * rotMat * transMat;
}

//-- Default pose skeleton update
void CModel::UpdateSkeleton(CBoneInstance* bone, const DirectX::XMMATRIX& parentTransform)
{
    if (!bone)
        return;
    bone->mGlobalTransform = (bone->isIK ? bone->mLocalTransformIK : bone->mLocalTransform) * parentTransform;
    DirectX::XMStoreFloat3(&bone->m_vWorldPosition, bone->mGlobalTransform.r[3]);
    bone->mRenderTransform = bone->mOffsetTransform * bone->mGlobalTransform;
    for (auto* child : bone->m_vChilds)
        UpdateSkeleton(child, bone->mGlobalTransform);
}

//-- Update transform if model attached to another model on scene
//-- TODO: that works but looks wierd, need to reformat
void CModel::UpdateTransformAttached(float dt, float animTime) {

    m_AttachData.m_pParent->Update(dt); //-- not so cheap, but works 
    m_AttachData.m_pParent->m_CurrentTime -= dt * m_AttachData.m_pParent->m_TicksPerSecond;

    //-- attached. NOTE: this relies on the parent's Update() having
    //-- already run this frame (so the parent bone's mGlobalTransform is
    //-- current) - make sure attached models are updated *after* their
    //-- parent. (fixed on top)
    DirectX::XMMATRIX parentTransform = m_AttachData.m_pParent->XFORM();
    bool haveParentBone = false;

    if (m_AttachData.m_pParent->m_Skeleton.m_BoneCount && !m_AttachData.m_ParentBone.empty()) {
        if (CBoneInstance* pParentBone = m_AttachData.m_pParent->m_Skeleton.GetBoneInstance(m_AttachData.m_ParentBone)) {
            parentTransform = pParentBone->mGlobalTransform; //-- already includes parent's XFORM(), see UpdateSkeleton/TraverseHierarchy
            haveParentBone = true;
        }
    }
    m_AttachData.m_ParentTransform = parentTransform;

    DirectX::XMMATRIX scaleMat = DirectX::XMMatrixScaling(
        m_AttachData.m_attachScale.x, m_AttachData.m_attachScale.y, m_AttachData.m_attachScale.z);
    DirectX::XMMATRIX rotMat =
        DirectX::XMMatrixRotationX(m_AttachData.m_attachRot.x) *
        DirectX::XMMatrixRotationY(m_AttachData.m_attachRot.y) *
        DirectX::XMMatrixRotationZ(m_AttachData.m_attachRot.z);
    DirectX::XMMATRIX transMat = DirectX::XMMatrixTranslation(
        m_AttachData.m_attachPos.x, m_AttachData.m_attachPos.y, m_AttachData.m_attachPos.z);
    DirectX::XMMATRIX offsetMat = scaleMat * rotMat * transMat;

    const std::string& childBoneName =
        m_AttachData.m_ChildBone.empty() ? m_Skeleton.GetRootBone()->m_sBoneName : m_AttachData.m_ChildBone;
    CBoneInstance* pChildBone = m_Skeleton.m_BoneCount ? m_Skeleton.GetBoneInstance(childBoneName) : nullptr;

    if (haveParentBone && pChildBone) {
        //-- bone-to-bone attach: figure out where OUR OWN attach bone sits
        //-- in OUR OWN local space (root at identity - i.e. unaffected by
        //-- m_trans, which is exactly what we're solving for), using the
        //-- model's current pose. Then solve m_trans so that bone lands
        //-- exactly on top of the parent's bone in world space. This is
        //-- what cancels out any mismatch between this model's mesh
        //-- origin and its own skeleton bind pose.

        //-- VlaGan: little fix to sync anims (enable if needed)
        /*if (m_pCurrentMotion && m_AttachData.m_pParent->m_pCurrentMotion) {

            float d1 = m_pCurrentMotion->Duration();
            float d2 = m_AttachData.m_pParent->m_pCurrentMotion->Duration();

            if (d1 < d2) {
                m_CurrentTime = m_AttachData.m_pParent->m_CurrentTime * d1 / d2;
                animTime = fmodf(m_CurrentTime, m_AnimationDuration);
            }
        }*/

        if (scene && scene->HasAnimations() && m_pCurrentMotion)
            TraverseHierarchy(animTime, scene->mRootNode->FindNode(rootName().c_str()), DirectX::XMMatrixIdentity());
        else if (m_pCurrentMotion)
            TraverseSkeleton(animTime, m_Skeleton.GetRootBone(), DirectX::XMMatrixIdentity());
        else
            UpdateSkeleton(m_Skeleton.GetRootBone(), DirectX::XMMatrixIdentity());

        DirectX::XMMATRIX childBoneLocal = pChildBone->mGlobalTransform;
        m_trans = DirectX::XMMatrixInverse(nullptr, childBoneLocal) * offsetMat * parentTransform;
    }
    else {
        //-- fallback: no matching bone pair found - just hang the whole
        //-- model off the parent bone/world transform with the manual offset
        m_trans = offsetMat * parentTransform;
    }
}

//-- update model skeleton and transform
void CModel::Update(float dt) {

    //-- restart anim
    if (m_AnimationDuration <= m_CurrentTime)
        m_CurrentTime = 0.f;

    m_CurrentTime += dt * m_TicksPerSecond; // seconds to ticks
    float animTime = fmodf(m_CurrentTime, m_AnimationDuration);

    if (!m_AttachData.valid())
        UpdateTransform();
    else
        UpdateTransformAttached(dt, animTime);

    //-- this (re)computes the skeleton using the now-final m_trans, so the
    //-- pre-pass above (if any) gets overwritten with the correct result
    if (m_Skeleton.m_BoneCount)
        UpdateSkeleton(m_Skeleton.GetRootBone(), XFORM());

    if (m_pCurrentMotion) { //-- else for simple visual rendering need to set not skinning shader

        if (scene && scene->HasAnimations())
        {
            //------------------------------------------------------------------------------
            //-- TODO: rootName method is trash, need to correctly 
            //-- implement skeleton hierarchy reading and CSkeleton class
            //-- then dont use assimp aiNode hierarchy (cuz that have lots of trash info)
            //------------------------------------------------------------------------------
            std::string root_bone = rootName(); //-- easyer for model changing
            TraverseHierarchy(animTime, scene->mRootNode->FindNode(root_bone.c_str()), XFORM());
        }
        else if (m_Skeleton.m_BoneCount)
        {
            //-- natively (.ogf) loaded model: no aiScene, walk CSkeleton's own tree instead
            TraverseSkeleton(animTime, m_Skeleton.GetRootBone(), XFORM());
        }
    }
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//-- VlaGan: Model Rendering
//----------------------------------------------------------------------------
void CModel::UpdateBones(ID3D11DeviceContext* context) {
    if (!m_Skeleton.m_BoneCount) return;

    // after TraverseHierarchy we have ready m_BoneInfo[i].finalTransformation matrices
    // lets send it to shader
    std::vector<DirectX::XMMATRIX> boneMatrices(MAX_BONES, DirectX::XMMatrixIdentity()/*m_Skeleton.m_vBones.size()*/);
    for (size_t i = 0; i < m_Skeleton.m_vBones.size(); ++i)
        boneMatrices[i] = DirectX::XMMatrixTranspose(m_Skeleton.m_vBones[i].mRenderTransform);

    // update bones buffer
    m_BoneBuffer.Update(context, 0, nullptr, boneMatrices.data(), 0, 0);
    // and send to vs shader
    m_BoneBuffer.VSSet(context, 2);
}

void CModel::Render(ID3D11DeviceContext* context, bool transparent)
{

	CRenderer* Render = &CRenderer::Get();

    //-- update camera and model matrices
    MatrixBuffer matrices;
    matrices.world = XMMatrixTranspose(m_trans);
    matrices.view = XMMatrixTranspose(Render->m_View);
    matrices.proj = XMMatrixTranspose(Render->m_Projection);
    matrices.lightVP = XMMatrixTranspose(Render->m_LightViewProj);

    Render->m_ConstantBuffer.Update(context, 0, nullptr, &matrices, 0, 0);
    Render->m_ConstantBuffer.VSSet(context, 0);

    if(m_Skeleton.m_BoneCount)
        UpdateBones(context);

    //-- render meshes
    for (auto& mesh : m_Meshes)
        if(mesh.IsTextureTransparent() == transparent)
            mesh.Render(context);
}

//-- render only depth to separate rt
void CModel::RenderShadowMap(ID3D11DeviceContext* context) {

    if (m_Skeleton.m_BoneCount)
        UpdateBones(context);

    for (auto& mesh : m_Meshes) 
        mesh.RenderSM(context);
}

//-- Blender-style selection -> inverted-hull outline
void CModel::RenderOutline(ID3D11DeviceContext* context, float thickness, DirectX::XMFLOAT3 color)
{
    CRenderer* Render = &CRenderer::Get();

    MatrixBuffer matrices;
    matrices.world = XMMatrixTranspose(m_trans);
    matrices.view = XMMatrixTranspose(Render->m_View);
    matrices.proj = XMMatrixTranspose(Render->m_Projection);
    matrices.lightVP = XMMatrixTranspose(Render->m_LightViewProj);

    Render->m_ConstantBuffer.Update(context, 0, nullptr, &matrices, 0, 0);
    Render->m_ConstantBuffer.VSSet(context, 0);

    if (m_Skeleton.m_BoneCount)
        UpdateBones(context);

    OutlineCB outlineData{ thickness, color };
    Render->m_OutlineBuffer.Update(context, 0, nullptr, &outlineData, 0, 0);
    Render->m_OutlineBuffer.VSSet(context, 3);
    Render->m_OutlineBuffer.PSSet(context, 3);

    for (auto& mesh : m_Meshes)
        mesh.RenderOutline(context);
}
//----------------------------------------------------------------------------
