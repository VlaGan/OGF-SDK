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
#include "../CMotion.h" // CMotion / BoneMotionData / KeyPosition / KeyRotation

//-- oriented bounding box: 3x3 rotation (row-major, matches Fmatrix33's i/j/k
//-- rows) + translate + halfsize - byte-exact with X-Ray's Fobb (xrCore/_obb.h)
struct SOgfObb
{
    DirectX::XMFLOAT3 rotationI{1.f, 0.f, 0.f}; // rotation matrix row 0
    DirectX::XMFLOAT3 rotationJ{0.f, 1.f, 0.f}; // rotation matrix row 1
    DirectX::XMFLOAT3 rotationK{0.f, 0.f, 1.f}; // rotation matrix row 2
    DirectX::XMFLOAT3 translate{};
    DirectX::XMFLOAT3 halfsize{};
};

//-- byte-exact with X-Ray's Fsphere (xrCore/_sphere.h)
struct SOgfSphere
{
    DirectX::XMFLOAT3 center{};
    float radius = 0.f;
};

//-- byte-exact with X-Ray's Fcylinder (xrCore/_cylinder.h)
struct SOgfCylinder
{
    DirectX::XMFLOAT3 center{};
    DirectX::XMFLOAT3 direction{};
    float height = 0.f;
    float radius = 0.f;
};

//-- which of the three shapes below is actually meaningful for this bone -
//-- matches OGSR-Engine's SBoneShape::ShapeType / OGF-tool's OGF_SHAPE_TYPE_*
enum class EOgfBoneShapeType : uint16_t
{
    None = 0,
    Box = 1,
    Sphere = 2,
    Cylinder = 3,
    Invalid = 0xFFFF,
};

//-- collision shape used for the bone by the physics/ragdoll system (ODE-style
//-- primitives) - byte-exact with X-Ray's SBoneShape (Layers/xrRender/bone.h)
struct SOgfBoneShape
{
    EOgfBoneShapeType type = EOgfBoneShapeType::Invalid;
    uint16_t flags = 0;
    SOgfObb box;
    SOgfSphere sphere;
    SOgfCylinder cylinder;
};

//-- one axis' worth of joint rotation limits (radians) + spring/damping
struct SOgfJointLimit
{
    DirectX::XMFLOAT2 limit{}; // min/max angle
    float springFactor = 1.f;
    float dampingFactor = 1.f;
};

//-- physics joint parameters - byte-exact with X-Ray's SJointIKData
//-- (xr_3da/bone.h) / OGF-tool's TOgfJointIKDataRawData. `frictionValid` is
//-- false for the two oldest IK-data chunk sub-versions, which never stored
//-- a friction value at all (see COgfLoader::LoadSkeleton for the version
//-- fallback chain) - left at its default (0) in that case.
struct SOgfIKData
{
    bool valid = false; // false if this bone had no OGF_S_IKDATA entry at all

    uint32_t jointType = 0xFFFFFFFFu; // OGF_JOINT_TYPE_INVALID
    SOgfJointLimit limits[3]; // X, Y, Z axis limits
    float springFactor = 0.f;
    float dampingFactor = 0.f;
    uint32_t ikFlags = 0;
    float breakForce = 0.f;
    float breakTorque = 0.f;

    bool frictionValid = false;
    float friction = 0.f;
};

//-- OGF_S_DESC: export/authoring metadata (source .max/.ogf name, tool used,
//-- owner, timestamps). Byte-exact with OGF-tool's Description class -
//-- including its "try 8-byte then fall back to 4-byte" timer heuristic,
//-- since older X-Ray SDK tools wrote 32-bit Unix timestamps while newer
//-- ones write 64-bit - the chunk carries no version tag to tell them apart,
//-- only the total chunk size (checked against both possible layouts).
struct SOgfDescription
{
    bool present = false; // false if the model has no OGF_S_DESC at all

    std::string sourceFile; // original authoring file (e.g. a .max scene)
    std::string exportTool; // tool name/version that exported this .ogf
    int64_t exportTime = 0; // unix timestamp
    std::string ownerName;
    int64_t creationTime = 0;
    std::string lastModifiedByTool;
    int64_t modifiedTime = 0;

    bool fourByteTimers = true; // which of the two timer layouts matched
};

//-- one skeleton bone, as read from OGF_S_BONE_NAMES / OGF_S_IKDATA
struct SOgfBoneDef
{
    std::string name;
    std::string parentName;
    int parentIndex = -1; // resolved by COgfLoader once every bone name is known

    //-- bind-pose local transform, X-Ray "XYZ" Euler convention (radians)
    DirectX::XMFLOAT3 rotation{};
    DirectX::XMFLOAT3 position{};

    //-- from OGF_S_BONE_NAMES: bounding box used for e.g. hit-detection/picking
    SOgfObb obb;

    //-- everything below is from OGF_S_IKDATA (per-bone, same order as
    //-- OGF_S_BONE_NAMES) - present whenever the skeleton has bind-pose data
    //-- at all, i.e. whenever `rotation`/`position` above are meaningful.
    std::string material; // game material name (surface sounds, etc.)
    SOgfBoneShape shape; // ragdoll/collision shape (box/sphere/cylinder)
    SOgfIKData ikData; // joint physics parameters
    float mass = 0.f;
    DirectX::XMFLOAT3 centerOfMass{};
};

//-- one renderable piece of geometry (either the whole model for MT_NORMAL,
//-- or one child of a hierarchical/skeletal model)
struct SOgfMeshDef
{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    //-- Tangent/binormal, parallel to `vertices` (same index, same count) -
    //-- kept separate from the Vertex struct on purpose: Vertex is the
    //-- render-side layout shared with CMesh/the D3D11 input assembler/HLSL,
    //-- and touching it means touching the whole render pipeline. These
    //-- exist purely so a future .ogf/.omf exporter doesn't have to
    //-- recompute tangent space for geometry that already had it.
    //--
    //-- Populated for skinned formats that store T/B in the file (2L/3L/4L
    //-- always do; 1L only for format_version 4, not format_version 3's
    //-- legacy 36-byte layout). Left EMPTY for static (MT_NORMAL) geometry -
    //-- the plain FVF vertex format never carries tangent/binormal at all,
    //-- so there is nothing to preserve; a future exporter that needs them
    //-- (e.g. to upgrade a static mesh into a bump-mapped one) has to compute
    //-- them algorithmically (standard position+normal+UV+triangle method).
    std::vector<DirectX::XMFLOAT3> tangents;
    std::vector<DirectX::XMFLOAT3> binormals;

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

    //-- names of referenced .omf files (from OGF_S_MOTION_REFS/2), without
    //-- path or extension - resolved and loaded separately via
    //-- COgfLoader::LoadMotions() since they live in their own files on disk
    std::vector<std::string> motionRefs;

    //-- decoded motions, filled by COgfLoader::LoadMotions() - already in the
    //-- exact shape CModel::m_vMotions expects, ready to hand over as-is
    std::vector<CMotion> motions;

    //-- OGF_S_USERDATA: an arbitrary script/config text blob some models
    //-- carry (e.g. attachment point definitions for weapons). Empty if absent.
    std::string userData;

    //-- OGF_S_DESC: authoring/export metadata. `present=false` if absent.
    SOgfDescription description;

    //-- OGF_S_LODS (or OGF3_LODS for format_version 3): a reference path to
    //-- this model's LOD replacement, e.g. "smart_terrains\lod\some_lod".
    //-- Empty if the model has no separate LOD variant.
    std::string lodPath;
};
