//----------------------------------------------------------------------------
//-- CMotion.cpp
//-- VlaGan: General class for motions && bone motions data
//----------------------------------------------------------------------------
#include "CMotion.h"

CMotion::CMotion(std::string name, float duration, float tps) {
    m_sMotionName = name;
    m_fDuration = duration;
    m_fTPS = tps;
}

//-- Getting bone motion data
BoneMotionData* CMotion::GetBoneMotion(std::string bname) {
    for (auto& bm : m_vBoneMotions)
        if (bname == bm.name)
            return &bm;

    return nullptr;
}

//-- adding bone motion data for needed bone
void CMotion::AddBoneMtData(BoneMotionData bmd) {
    m_vBoneMotions.push_back(bmd);
}
