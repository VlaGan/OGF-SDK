//----------------------------------------------------------------------------
//-- CSkeleton.cpp
//-- VlaGan: Skeleton class implementation
//----------------------------------------------------------------------------
#include "CSkeleton.h"
#include <functional>
#include "../AssimpHelper/AssimpFunctions.h"


extern void LogMsg(const char* fmt, ...);

//-- Constructor / Destructor
CSkeleton::CSkeleton() {
    m_BoneCount = (u32)0;
    m_BonesNode = nullptr;
    m_vBones.clear();
}

CSkeleton::~CSkeleton() { Release(); }

void CSkeleton::Release() {
    m_BoneCount = (u32)0;
    m_BonesNode = nullptr;
    m_vBones.clear();
}


aiNode* GetNodeByName(aiNode* node, aiString name) {
    if (!node)
        return nullptr;

    if (node->mName == name)
        return node;

    for (unsigned int i{}; i < node->mNumChildren; ++i)
        return (GetNodeByName(node->mChildren[i], name));

    return nullptr;
}

//-- maybe nice idea - separate header for assimp tweaks like this
//-- finding root bone node (yeah)
aiNode* GetRootBoneNode(const aiScene* scene) {
    for (unsigned i{}; i < scene->mNumMeshes; ++i) {
        const aiMesh* mesh = scene->mMeshes[i];
        for (unsigned j{}; j < mesh->mNumBones; ++j) {
            aiBone* bone = mesh->mBones[j];
            if (bone->mArmature) {
                std::string test = "~!-Bone armature ref node = [%s] = ";
                test += bone->mArmature->mName.C_Str();
                LogMsg(test.c_str());
                

                aiString nodename("Root");
                if (auto rnode = GetNodeByName(scene->mRootNode, nodename); rnode)
                    return rnode;

                //-- wierd, but there are models which armature node ptr == root bone
                std::string name = bone->mArmature->mName.C_Str();
                for (char& c : name)
                    c = std::tolower(static_cast<unsigned char>(c));

                std::string armature = "armature";
                if (name.find(armature) != std::string::npos)
                    return bone->mArmature->mChildren[0]; //-- root bone (if have armature tmp node)
                
                return bone->mArmature;
            }
        }
    }

    return nullptr;
}

//-- load skeleton
//-- finally correct method to calculate bone data and load skeleton
void CSkeleton::Load(const aiScene* scene) {

    LogMsg("//-----------------------------------------------------");
    LogMsg("--CSkeleton::LoadSkeleton - Loading Skeleton");
    LogMsg("//-----------------------------------------------------");

    const aiNode* pRootBN = GetRootBoneNode(scene);
    if (!pRootBN) {
        LogMsg("![Cant find root bone node] try flag [aiProcess_PopulateArmatureData] in scene loading!");
        LogMsg("//-----------------------------------------------------");
        return;
    }

    //-- fill bones vector
    std::function<void(const aiNode*)> formSkeleton = 
        [&](const aiNode* nodeAs) {
        //if (!nodeAs) return;

        CBoneInstance bi;
        bi.m_sBoneName = nodeAs->mName.C_Str();
        bi.m_bBoneID = m_BoneCount++;
        bi.mIKTrans = AssimpToXMMatrix(nodeAs->mTransformation);
        bi.mLocalTransform = AssimpToXMMatrix(nodeAs->mTransformation); // local
        bi.mLocalTransformIK = bi.mLocalTransform;
        bi.mGlobalTransform = AssimpToXMMatrix(calcTrans(nodeAs, pRootBN)); // global

        DirectX::XMStoreFloat3(&bi.m_vCurrentPos, bi.mGlobalTransform.r[3]); // oi trash (need global trans)
        bi.m_vPrevPos = bi.m_vCurrentPos;

        //if (auto pAiBone = GetAiBone(scene, bi.m_sBoneName); pAiBone) {
        //    bi.mOffsetTransform = AssimpToXMMatrix(pAiBone->mOffsetMatrix); // offset
        //    bi.m_bToMesh = true;
        //}
        //else
        bi.mOffsetTransform = DirectX::XMMatrixInverse(nullptr, bi.mGlobalTransform); // offset

        bi.mRenderTransform = bi.mGlobalTransform * bi.mOffsetTransform; //-- default skeleton pose

        m_vBones.push_back(bi);

        for (unsigned i{}; i < nodeAs->mNumChildren; ++i)
            formSkeleton(nodeAs->mChildren[i]);

    };
    formSkeleton(pRootBN);
    LogMsg("//-----------------------------------------------------");

    //-- fill bones graph hierarchy
    std::function<void(CBoneInstance*, const aiNode*)> formSkelTree = [&](CBoneInstance* pbi, const aiNode* node) {
 
        for (unsigned i{}; i < node->mNumChildren; ++i) {
            pbi->m_vChilds.push_back(GetBoneInstance(node->mChildren[i]->mName.C_Str()));
            ///LogMsg("--[%d] - [%s]", pbi->m_vChilds.back()->m_bBoneID, pbi->m_vChilds.back()->m_sBoneName.c_str());
        }
        for (auto& ch : pbi->m_vChilds) {
            DirectX::XMVECTOR parentpos = DirectX::XMLoadFloat3(&pbi->m_vCurrentPos);
            DirectX::XMVECTOR childpos = DirectX::XMLoadFloat3(&ch->m_vCurrentPos);
            
            //-- bone->parent lenght
            pbi->m_fBoneLenght = DirectX::XMVectorGetX(DirectX::XMVector3Length(DirectX::XMVectorSubtract(childpos, parentpos)));
            ch->m_pParent = pbi;
        }

        for (unsigned i{}; i < node->mNumChildren; ++i)
            formSkelTree(pbi->m_vChilds[i], node->mChildren[i]);

    };
    //-- better way bacause there was data loss while pushing child and init parent
    m_BonesNode = &m_vBones[0];
    m_BonesNode->m_pParent = nullptr;
    formSkelTree(m_BonesNode, pRootBN);

    std::function<void(CBoneInstance*)> displatRecursive = [&](CBoneInstance* pbi) {
        if (!pbi) {
            LogMsg("!!!!BAD");
            return;
        }
        const char* parent_nm = pbi->m_pParent ? pbi->m_pParent->m_sBoneName.c_str() : "ROOT";
        LogMsg("--[%d] - [%s] - m_pParent[%s]", pbi->m_bBoneID, pbi->m_sBoneName.c_str(), parent_nm);
        for (auto& ch : pbi->m_vChilds)
            displatRecursive(ch);
    };
    //displatRecursive(m_BonesNode);

    //-- correct(better) way to calc bone iverse bind pose for shkinning
    for (auto& bone : m_vBones)
    {
        if (auto paib = GetAiBone(scene, bone.m_sBoneName); paib)
            bone.mOffsetTransform = AssimpToXMMatrix(paib->mOffsetMatrix);
        else
            if (bone.m_pParent)
                bone.mOffsetTransform = bone.m_pParent->mOffsetTransform * DirectX::XMMatrixInverse(nullptr, bone.mLocalTransform);
    }

    return;

    //---------------------------------------------------------------------------------
    //-- trying to compare aiBone and aiNode bone data (just test thing)
    //-- ! transposed node trans 3x3 equal bone offset 3x3
    aiBone* pBone = GetAiBone(scene, GetRootBone()->m_vChilds[0]->m_sBoneName);
    aiNode* pNode = scene->mRootNode->FindNode(pBone->mName.C_Str());
    LogMsg("~~ aiBone vs aiNode transform -------------------------");
    LogMsg("~~ Bone [%d]", 1);
    LogMsg("~~ pBone[%s]", pBone->mName.C_Str());
    for (int i{}; i < 4; ++i) {
        DirectX::XMFLOAT4 row;
        DirectX::XMStoreFloat4(&row, AssimpToXMMatrix(pBone->mOffsetMatrix).r[i]);
        LogMsg("~[%d] - [%.2f, %.2f, %.2f, %.2f]", i, VPUSH4(row));
    }
    LogMsg("\n~~ pNode[%s]", pNode->mName.C_Str());
    for (int i{}; i < 4; ++i) {
        aiMatrix4x4 mt = calcTrans(pNode, pRootBN);
        DirectX::XMMATRIX mat = DirectX::XMMatrixInverse(nullptr, AssimpToXMMatrix(mt));
        DirectX::XMFLOAT4 row2;
        DirectX::XMStoreFloat4(&row2, mat.r[i]);
        LogMsg("~[%d] - [%.2f, %.2f, %.2f, %.2f]", i, VPUSH4(row2));
    }
    //---------------------------------------------------------------------------------

    LogMsg("//-----------------------------------------------------");
    LogMsg("--CSkeleton::Load succesfully completed!");
    LogMsg("//-----------------------------------------------------");
}
