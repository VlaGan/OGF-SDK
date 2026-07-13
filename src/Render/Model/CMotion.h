//----------------------------------------------------------------------------
//-- CMotion.h
//-- VlaGan: General class for motions && bone motions data .header
//----------------------------------------------------------------------------
#pragma once

#include <DirectXMath.h>
#include <string>
#include <vector>

//-- key position for bone
struct KeyPosition {
    DirectX::XMFLOAT3 position;
    float timeStamp;
};

//-- key rotation for bone
struct KeyRotation {
    DirectX::XMFLOAT4 rotation;
    float timeStamp;
};

//-- key scale for bone
struct KeyScale {
    DirectX::XMFLOAT3 scale;
    float timeStamp;
};

//-- key pos, rot, scale for single bone in 1 motion
struct BoneMotionData {
    std::string name;
    std::vector<KeyPosition> positions;
    std::vector<KeyRotation> rotations;
    std::vector<KeyScale> scales;
};

//-- motion class
class CMotion {

public:
    CMotion(std::string name, float duration, float tps = 30.f);
    
    std::string GetName() { return m_sMotionName; }
    float Duration() { return m_fDuration; }
    float TPS() { return m_fTPS; }

    void SetName(std::string name) { m_sMotionName = name; }
    void SetDuration(float duration) { m_fDuration = duration; }
    void SetTPS(float tps) { m_fTPS = tps; }
    
    void AddBoneMtData(BoneMotionData bmd);
    BoneMotionData* GetBoneMotion(std::string bname);

    std::string m_sMotionName;               //-- motion name

    float m_fDuration{};                     //-- motion duration
    float m_fTPS{};                          //-- Ticks Per Second

    std::vector<BoneMotionData> m_vBoneMotions;    //-- bone motions vector

};
