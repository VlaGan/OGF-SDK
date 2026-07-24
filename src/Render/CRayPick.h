//----------------------------------------------------------------------------
//-- CRayPick.h
//-- VlaGan: ray-casting / mouse-picking utilities against scene models
//----------------------------------------------------------------------------
#pragma once
#include <DirectXMath.h>

class CModel;
class CScene;
class CCamera;

//-- world-space ray. dir does not need to be normalized - RaycastModel/
//-- RaycastScene both parametrize hits as origin + t*dir internally and
//-- convert to a real world-space distance for you (see SRayHit::distance).
struct SRay {
    DirectX::XMVECTOR origin;
    DirectX::XMVECTOR dir;
};

struct SRayHit {
    CModel* model = nullptr;
    float distance = 0.0f;         // world-space distance from ray.origin
    DirectX::XMFLOAT3 worldPos{};  // world-space hit point
};

namespace RayPick {

    //-- builds a world-space pick ray through normalized device coordinates
    //-- (ndcX/ndcY each in [-1, 1], NDC-up convention) via the camera's
    //-- current view/projection.
    SRay RayFromNDC(CCamera* camera, float ndcX, float ndcY, float aspectRatio);

    //-- convenience wrapper: builds a ray from a point in *screen* space
    //-- (e.g. mouse position relative to a viewport image's top-left
    //-- corner, screen-down Y) plus that viewport's pixel size.
    SRay RayFromScreenPoint(CCamera* camera, float screenX, float screenY, float vpWidth, float vpHeight);

    //-- per-triangle ray/model test. Animation-aware: if the model has a
    //-- skeleton, vertices are re-skinned on the CPU against the model's
    //-- *current* pose before testing, so results match what's on screen
    //-- even mid-animation (see CRayPick.cpp for details, including why
    //-- skinned models are tested in world space and non-skinned ones in
    //-- local space).
    bool RaycastModel(CModel* model, const SRay& ray, SRayHit& outHit);

    //-- tests every model in the scene, returns the closest hit (by
    //-- world-space distance). Returns false (outHit left untouched) if
    //-- the ray didn't hit anything.
    bool RaycastScene(CScene& scene, const SRay& ray, SRayHit& outHit);

}
