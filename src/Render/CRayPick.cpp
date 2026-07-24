//----------------------------------------------------------------------------
//-- CRayPick.cpp
//-- VlaGan: ray-casting / mouse-picking utilities against scene models
//----------------------------------------------------------------------------
#define NOMINMAX

#include "CRayPick.h"
#include "CScene.h"
#include "Model/CModel.h"
#include "../Core/CCamera.h"
#include <cfloat>
#include <algorithm>
#include <vector>


//-- Mirrors the GPU skinning 
DirectX::XMVECTOR SkinnedVertexPos(const CModel* model, const Vertex& v)
{
	if (!model->m_Skeleton.m_BoneCount)
		return XMLoadFloat3(&v.position);

	DirectX::XMVECTOR p = XMLoadFloat3(&v.position);
	DirectX::XMVECTOR skinned = DirectX::XMVectorZero();
	bool anyWeight = false;

	for (int i = 0; i < MAX_BONE_INFLUENCE; ++i)
	{
		float w = v.weights[i];
		if (!w) continue;

		int boneId = v.boneIDs[i];
		if (boneId < 0 || (size_t)boneId >= model->m_Skeleton.m_vBones.size()) continue;

		const DirectX::XMMATRIX& M = model->m_Skeleton.m_vBones[boneId].mRenderTransform;
		skinned = DirectX::XMVectorAdd(skinned, DirectX::XMVectorScale(XMVector3TransformCoord(p, M), w));
		anyWeight = true;
	}

	return anyWeight ? skinned : p;
}

//-- double-sided ray/triangle test (Moller-Trumbore). orig/dir and the
//-- triangle must be in the same space - RaycastModel below passes
//-- world-space for skinned models, local-space for everything else
//-- (see SkinnedVertexPos above for why). Returns the ray parameter t on hit.
bool RayIntersectsTriangle(DirectX::FXMVECTOR orig, DirectX::FXMVECTOR dir,
	const DirectX::XMFLOAT3& v0, const DirectX::XMFLOAT3& v1, const DirectX::XMFLOAT3& v2, float& outT)
{
	DirectX::XMVECTOR p0 = XMLoadFloat3(&v0);
	DirectX::XMVECTOR p1 = XMLoadFloat3(&v1);
	DirectX::XMVECTOR p2 = XMLoadFloat3(&v2);

	DirectX::XMVECTOR e1 = DirectX::XMVectorSubtract(p1, p0);
	DirectX::XMVECTOR e2 = DirectX::XMVectorSubtract(p2, p0);

	DirectX::XMVECTOR pvec = DirectX::XMVector3Cross(dir, e2);
	float det = DirectX::XMVectorGetX(DirectX::XMVector3Dot(e1, pvec));

	//-- abs(det): double-sided, so backfaces (e.g. inside of a sleeve/glove) are pickable too
	if (fabsf(det) < 1e-8f)
		return false;

	float invDet = 1.0f / det;

	DirectX::XMVECTOR tvec = DirectX::XMVectorSubtract(orig, p0);
	float u = DirectX::XMVectorGetX(DirectX::XMVector3Dot(tvec, pvec)) * invDet;
	if (u < 0.0f || u > 1.0f)
		return false;

	DirectX::XMVECTOR qvec = DirectX::XMVector3Cross(tvec, e1);
	float v = DirectX::XMVectorGetX(DirectX::XMVector3Dot(dir, qvec)) * invDet;
	if (v < 0.0f || u + v > 1.0f)
		return false;

	float t = DirectX::XMVectorGetX(DirectX::XMVector3Dot(e2, qvec)) * invDet;
	if (t < 0.0f)
		return false; // triangle is behind the ray origin

	outT = t;
	return true;
}

//-- cheap reject test so we don't walk every triangle of every mesh on
//-- every raycast; bbox and ray must be in the same space as each other
//-- (world or local - see RayIntersectsTriangle above).
bool RayIntersectsAABB(DirectX::FXMVECTOR orig, DirectX::FXMVECTOR dir, const DirectX::XMFLOAT3& bmin, const DirectX::XMFLOAT3& bmax)
{
	DirectX::XMFLOAT3 o, d;
	DirectX::XMStoreFloat3(&o, orig);
	DirectX::XMStoreFloat3(&d, dir);

	float tmin = -FLT_MAX, tmax = FLT_MAX;
	float omin[3] = { o.x, o.y, o.z }, dmin[3] = { d.x, d.y, d.z };
	float bmn[3] = { bmin.x, bmin.y, bmin.z }, bmx[3] = { bmax.x, bmax.y, bmax.z };

	for (int i = 0; i < 3; ++i)
	{
		if (fabsf(dmin[i]) < 1e-8f)
		{
			if (omin[i] < bmn[i] || omin[i] > bmx[i])
				return false;
		}
		else
		{
			float t1 = (bmn[i] - omin[i]) / dmin[i];
			float t2 = (bmx[i] - omin[i]) / dmin[i];
			if (t1 > t2) std::swap(t1, t2);
			tmin = std::max(tmin, t1);
			tmax = std::min(tmax, t2);
			if (tmin > tmax)
				return false;
		}
	}
	return tmax >= 0.0f;
}



SRay RayPick::RayFromNDC(CCamera* camera, float ndcX, float ndcY, float aspectRatio)
{
	DirectX::XMMATRIX view = camera->GetViewMatrix();
	DirectX::XMMATRIX proj = camera->GetProjectionMatrix(aspectRatio);
	DirectX::XMMATRIX invViewProj = XMMatrixInverse(nullptr, XMMatrixMultiply(view, proj));

	//-- unproject the near/far points of the pick ray straight out of NDC space
	DirectX::XMVECTOR nearPt = XMVector3TransformCoord(DirectX::XMVectorSet(ndcX, ndcY, 0.0f, 1.0f), invViewProj);
	DirectX::XMVECTOR farPt = XMVector3TransformCoord(DirectX::XMVectorSet(ndcX, ndcY, 1.0f, 1.0f), invViewProj);

	SRay ray;
	ray.origin = nearPt;
	ray.dir = DirectX::XMVectorSubtract(farPt, nearPt);
	return ray;
}

SRay RayPick::RayFromScreenPoint(CCamera* camera, float screenX, float screenY, float vpWidth, float vpHeight)
{
	//-- screen (down) -> NDC (up)
	float ndcX = (2.0f * screenX / vpWidth) - 1.0f;
	float ndcY = 1.0f - (2.0f * screenY / vpHeight);

	return RayFromNDC(camera, ndcX, ndcY, vpWidth / vpHeight);
}

bool RayPick::RaycastModel(CModel* model, const SRay& ray, SRayHit& outHit)
{
	if (!model)
		return false;

	DirectX::XMMATRIX world = model->XFORM();

	//-- Skinned models bake XFORM() straight into every bone's mRenderTransform
	//-- Non-skinned models are still authored/stored in local (bind-pose)
	const bool isSkinned = model->m_Skeleton.m_BoneCount > 0;

	DirectX::XMVECTOR testOrigin = ray.origin;
	DirectX::XMVECTOR testDir = ray.dir;

	if (!isSkinned)
	{
		DirectX::XMMATRIX invWorld = XMMatrixInverse(nullptr, world);
		testOrigin = XMVector3TransformCoord(ray.origin, invWorld);
		testDir = XMVector3TransformNormal(ray.dir, invWorld); // intentionally not normalized
	}

	bool hitAny = false;
	float closestT = FLT_MAX;

	for (auto& mesh : model->m_Meshes)
	{
		if (mesh.m_vVertices.empty() || mesh.m_vIndices.size() < 3)
			continue;

		//-- mirror the current GPU pose so the ray tests against what's
		//-- actually on screen right now (animated meshes), not the
		//-- bind pose baked into m_vVertices
		std::vector<DirectX::XMFLOAT3> pose(mesh.m_vVertices.size());
		for (size_t vi = 0; vi < mesh.m_vVertices.size(); ++vi)
			DirectX::XMStoreFloat3(&pose[vi], SkinnedVertexPos(model, mesh.m_vVertices[vi]));

		DirectX::XMFLOAT3 bmin = pose[0];
		DirectX::XMFLOAT3 bmax = bmin;
		for (auto& p : pose)
		{
			bmin.x = std::min(bmin.x, p.x); bmax.x = std::max(bmax.x, p.x);
			bmin.y = std::min(bmin.y, p.y); bmax.y = std::max(bmax.y, p.y);
			bmin.z = std::min(bmin.z, p.z); bmax.z = std::max(bmax.z, p.z);
		}

		if (!RayIntersectsAABB(testOrigin, testDir, bmin, bmax))
			continue;

		for (size_t i = 0; i + 2 < mesh.m_vIndices.size(); i += 3)
		{
			const DirectX::XMFLOAT3& v0 = pose[mesh.m_vIndices[i + 0]];
			const DirectX::XMFLOAT3& v1 = pose[mesh.m_vIndices[i + 1]];
			const DirectX::XMFLOAT3& v2 = pose[mesh.m_vIndices[i + 2]];

			float t;
			if (RayIntersectsTriangle(testOrigin, testDir, v0, v1, v2, t) && t < closestT)
			{
				closestT = t;
				hitAny = true;
			}
		}
	}

	if (!hitAny)
		return false;

	DirectX::XMVECTOR hitPoint = DirectX::XMVectorAdd(testOrigin, DirectX::XMVectorScale(testDir, closestT));

	//-- testOrigin/testDir/hitPoint are already world-space for skinned
	//-- models; for non-skinned ones they're local-space and still need
	//-- to be brought back to world space for a distance comparable
	//-- across other models
	DirectX::XMVECTOR worldHit = isSkinned ? hitPoint : XMVector3TransformCoord(hitPoint, world);

	outHit.model = model;
	outHit.distance = DirectX::XMVectorGetX(DirectX::XMVector3Length(DirectX::XMVectorSubtract(worldHit, ray.origin)));
	XMStoreFloat3(&outHit.worldPos, worldHit);
	return true;
}

//-- raycast the whole scene models
bool RayPick::RaycastScene(CScene& scene, const SRay& ray, SRayHit& outHit)
{
	bool found = false;
	SRayHit best;
	float closestDist = FLT_MAX;

	for (auto& model : scene.GetModels())
	{
		SRayHit hit;
		if (RaycastModel(model, ray, hit) && hit.distance < closestDist)
		{
			closestDist = hit.distance;
			best = hit;
			found = true;
		}
	}

	if (found)
		outHit = best;
	return found;
}
