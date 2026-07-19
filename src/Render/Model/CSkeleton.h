//----------------------------------------------------------------------------
//-- CSkeleton.h
//-- VlaGan: Skeleton class implementation interface
//----------------------------------------------------------------------------
#pragma once
#include <vector>
#include <string>
#include <DirectXMath.h>
#include "../../_defines.h"
#include <wtypes.h>
#include <unordered_map>
#include <assimp/scene.h>
#include "OGF/COgfModel.h"

struct CBoneInstance {
	UINT m_bBoneID;					       // bone ID
	std::string m_sBoneName;			   // bone name
	bool m_bToMesh{};

	// test IK data
	DirectX::XMMATRIX mIKTrans;
	DirectX::XMFLOAT3 m_vWorldPosition;
	DirectX::XMMATRIX mLocalTransformIK{};
	bool isIK{};

	// maybe its correct dispay tweak when there are no motions
	// so default pose is a solutin 

	DirectX::XMMATRIX mLocalTransform;     // local bone transform
	DirectX::XMMATRIX mGlobalTransform;    // from root to bone (to avoid recalculation)
	DirectX::XMMATRIX mOffsetTransform;    // mesh offset(!) skeleton bone transform matrix
	DirectX::XMMATRIX mRenderTransform;    // final matrix for render (multiplyed by bind pose offset, dont use to get pos!)
	
	CBoneInstance* m_pParent{};            // Bone parent
	std::vector<CBoneInstance*> m_vChilds; // Child bones


	DirectX::XMFLOAT3 m_vCurrentPos;    // Current position (for simulation) ѕоточна позиц≥€ (симульована)
	DirectX::XMFLOAT3 m_vPrevPos;       // Previous position (Verlet)
	float m_fStiffness = 0.25f;         // Stiffness (0.1Ц0.5)
	float m_fDamping = 0.5f;            // Damping (0.1Ц0.9)
	float m_fMass = 0.5f;               // Bone mass
	bool m_bSimulatePhysics{};          // Simulate physics for this bone (hair)
	float m_fBoneLenght{};              // Bone lenght from bone to parent
};

class CSkeleton {

public:
	CSkeleton();
	~CSkeleton();
	void Release();

	//-- load skeleton
	void Load(const aiScene* scene);

	//-- native .ogf skeleton loading (bones already come with a resolved
	//-- parentIndex and a bind-pose rotation/position, see COgfLoader)
	void LoadFromOGF(const std::vector<SOgfBoneDef>& ogfBones);

	//-- default methods
	INL CBoneInstance* GetRootBone() { return m_BonesNode ? m_BonesNode : &m_vBones[0]; }
	INL CBoneInstance* GetBoneInstance(u32 bid) { return bid >= 0 && bid < m_BoneCount ? &m_vBones[bid] : nullptr; }
	INL CBoneInstance* GetBoneInstance(std::string name) {
		for (auto& bi : m_vBones)
			if (bi.m_sBoneName == name)
				return &bi;
		return nullptr;
	}
	INL CBoneInstance* GetBoneInstance(const char* name) {
		for (auto& bi : m_vBones)
			if (bi.m_sBoneName == name)
				return &bi;
		return nullptr;
	}


	//-------------------------------------------------------------
	u32 m_BoneCount{};
	std::vector<CBoneInstance> m_vBones;
	CBoneInstance* m_BonesNode;
};
