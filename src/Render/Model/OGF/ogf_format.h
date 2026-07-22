//----------------------------------------------------------------------------
//-- ogf_format.h
//-- Native .ogf (X-Ray / OGSR engine) model format definitions.
//--
//-- Ported (byte-exact) from OGSR-Engine, see:
//--   https://github.com/OGSR/OGSR-Engine
//--   ogsr_engine/xr_3da/fmesh.h  (chunk ids, header, MT_* visual types)
//--   ogsr_engine/xr_3da/bone.h   (vertBoned1W..4W raw skinned vertex layout)
//--
//-- This header has NO dependency on the OGSR engine itself - it only
//-- describes the on-disk binary layout of an .ogf file.
//----------------------------------------------------------------------------
#pragma once
#include <cstdint>

using ogf_u8 = uint8_t;
using ogf_u16 = uint16_t;
using ogf_u32 = uint32_t;

//-- top level visual type, stored in ogf_header::type (fmesh.h: enum MT)
enum EOgfModelType : ogf_u8
{
    MT_NORMAL = 0,
    MT_HIERRARHY = 1,
    MT_PROGRESSIVE = 2,
    MT_SKELETON_ANIM = 3,
    MT_SKELETON_GEOMDEF_PM = 4,
    MT_SKELETON_GEOMDEF_ST = 5,
    MT_LOD = 6,
    MT_TREE_ST = 7,
    MT_PARTICLE_EFFECT = 8,
    MT_PARTICLE_GROUP = 9,
    MT_SKELETON_RIGID = 10,
    MT_TREE_PM = 11,
    MT_3DFLUIDVOLUME = 12,
};

//-- chunk identifiers (fmesh.h: enum OGF_Chuncks)
enum EOgfChunk : ogf_u32
{
    OGF_HEADER = 1,
    OGF_TEXTURE = 2,
    OGF_VERTICES = 3,
    OGF_INDICES = 4,
    OGF_P_MAP = 5,
    OGF_SWIDATA = 6,
    OGF_VCONTAINER = 7,
    OGF_ICONTAINER = 8,
    OGF_CHILDREN = 9,
    OGF_CHILDREN_L = 10,
    OGF_LODDEF2 = 11,
    OGF_TREEDEF2 = 12,
    OGF_S_BONE_NAMES = 13,
    OGF_S_MOTIONS = 14,
    OGF_S_SMPARAMS = 15,
    OGF_S_IKDATA = 16,
    OGF_S_USERDATA = 17,
    OGF_S_DESC = 18,
    OGF_S_MOTION_REFS = 19,
    OGF_SWICONTAINER = 20,
    OGF_GCONTAINER = 21,
    OGF_FASTPATH = 22,
    OGF_S_LODS = 23,
    OGF_S_MOTION_REFS2 = 24,
};

//-- "link count" markers stored as dwVertType at the start of a skinned
//-- OGF_VERTICES chunk (fmesh.h: enum OGF_SkeletonVertType).
//-- Older files sometimes store the plain values 1..4 instead - both are handled.
enum EOgfSkeletonVertType : ogf_u32
{
    OGF_VERTEXFORMAT_FVF_1L = 1u * 0x12071980u,
    OGF_VERTEXFORMAT_FVF_2L = 2u * 0x12071980u,
    OGF_VERTEXFORMAT_FVF_NL = 3u * 0x12071980u,
    OGF_VERTEXFORMAT_FVF_3L = 4u * 0x12071980u,
    OGF_VERTEXFORMAT_FVF_4L = 5u * 0x12071980u,
};

constexpr ogf_u8 OGF_FORMAT_VERSION = 4; // xrOGF_FormatVersion, the "release" (CS/CoP) format

//----------------------------------------------------------------------------
//-- format_version == 3 (Shadow of Chornobyl / pre-release build era, roughly
//-- builds 1469-1865) uses a DIFFERENT chunk id numbering than the "release"
//-- format above (format_version == 4, used by CS/CoP and by OGSR-Engine).
//-- Ported from https://github.com/VaIeroK/OGF-tool (Classes/Model.cs).
//--
//-- format_version == 3 also has a *shorter* OGF_HEADER (no bbox/bsphere -
//-- see COgfLoader::LoadVisual) and, only for single-link (1L) skinned
//-- vertices, a shorter vertex struct without tangent/binormal - see
//-- ogf_vert_1w_legacy below.
//----------------------------------------------------------------------------
enum EOgfChunkV3 : ogf_u32
{
    OGF3_TEXTURE_L = 3,
    OGF3_CHILD_REFS = 5,
    OGF3_BBOX = 6,
    OGF3_VERTICES = 7,
    OGF3_INDICES = 8,
    OGF3_LODDATA = 9,
    OGF3_VCONTAINER = 10,
    OGF3_BSPHERE = 11,
    OGF3_CHILDREN_L = 12,
    OGF3_DPATCH = 15,
    OGF3_LODS = 16,
    OGF3_CHILDREN = 17,
    OGF3_S_SMPARAMS = 18,
    OGF3_ICONTAINER = 19,
    OGF3_LODDEF2 = 21,
    OGF3_TREEDEF2 = 22,
    OGF3_S_IKDATA_0 = 23, // oldest: no per-bone version field, Import block = 60 bytes
    OGF3_S_USERDATA = 24,
    OGF3_S_IKDATA = 25,   // no per-bone version field, Import block = 72 bytes
    OGF3_S_DESC = 27,
    OGF3_S_IKDATA_2 = 28, // has a per-bone version field, same layout as OGF4_S_IKDATA (Import = 76 bytes)
    OGF3_S_MOTION_REFS = 29,
};

//-- format_version 3 had at least two different revisions of the SMPARAMS /
//-- MOTIONS chunk ids across SDK builds; try these as a fallback if the
//-- "canonical" OGF_S_SMPARAMS(15)/OGF_S_MOTIONS(14) aren't found.
constexpr ogf_u32 OGF3_S_SMPARAMS_NEW = 20; // build 1472-1865

//-- per motion-definition behavior flags, stored as the u32 read right before
//-- CMotionDef in OGF_S_SMPARAMS - byte-exact with OGSR-Engine's xr_3da/motion.h
//-- ESMFlags enum. esmUseFootSteps/esmRootMover/esmSyncPart are the ones most
//-- relevant to "trigger something during this animation" style tricks.
enum EOgfMotionDefFlags : ogf_u16
{
    OGF_ESM_FX = 1 << 0, // one-shot effect motion (vs. a looping cycle)
    OGF_ESM_STOP_AT_END = 1 << 1, // don't loop back to frame 0 when it finishes
    OGF_ESM_NO_MIX = 1 << 2, // don't blend with other motions while playing
    OGF_ESM_SYNC_PART = 1 << 3, // bone_or_part below is a partition id, not a bone id
    OGF_ESM_USE_FOOT_STEPS = 1 << 4, // sync footstep sounds/marks to this motion
    OGF_ESM_ROOT_MOVER = 1 << 5, // motion drives the root bone's world position
    OGF_ESM_IDLE = 1 << 6,
    OGF_ESM_USE_WEAPON_BONE = 1 << 7,
};

constexpr ogf_u16 OGF_SMPARAMS_VERSION = 4; // xrOGF_SMParamsVersion - motion_marks only exist at this version or newer

constexpr ogf_u32 OGF3_S_MOTIONS_NEW = 26; // build 1616-1865

//----------------------------------------------------------------------------
//-- raw compressed keyframe structs stored inside OGF_S_MOTIONS, byte-exact
//-- with OGSR-Engine's xr_3da/SkeletonMotions.h (CKeyQR/CKeyQT8/CKeyQT16) and
//-- dequantized exactly like Layers/xrRender/AnimationKeyCalculate.h.
//----------------------------------------------------------------------------
#pragma pack(push, 1)
struct ogf_key_qr // quantized rotation: component = raw / OGF_KEY_QUANT (quaternion x,y,z,w)
{
    int16_t x, y, z, w;
};
struct ogf_key_qt8 // quantized translation, 8-bit: component = raw * sizeT + initT
{
    int8_t x, y, z;
};
struct ogf_key_qt16 // quantized translation, 16-bit: component = raw * sizeT + initT
{
    int16_t x, y, z;
};
#pragma pack(pop)

constexpr float OGF_KEY_QUANT = 32767.0f;
constexpr float OGF_KEY_QUANT_I = 1.0f / OGF_KEY_QUANT;
constexpr float OGF_SAMPLE_FPS = 30.0f; // fixed keyframe rate for all X-Ray skeleton motions

//-- per-bone, per-motion flags byte in OGF_S_MOTIONS (SkeletonMotions.h: flTKeyPresent/flRKeyAbsent/flTKey16IsBit)
enum EOgfMotionKeyFlags : ogf_u8
{
    OGF_MOTION_FL_T_KEY_PRESENT = 1 << 0, // translation is keyframed (else: single constant Fvector3)
    OGF_MOTION_FL_R_KEY_ABSENT = 1 << 1, // rotation is a single constant key (else: keyframed)
    OGF_MOTION_FL_T_KEY_16BIT = 1 << 2, // translation keys are ogf_key_qt16 (else: ogf_key_qt8)
};

//-- top level visual type for format_version == 3 - numerically different
//-- from EOgfModelType (format_version == 4) above!
enum EOgfModelTypeV3 : ogf_u8
{
    MT3_NORMAL = 0,
    MT3_HIERRARHY = 1,
    MT3_PROGRESSIVE = 2,
    MT3_SKELETON_GEOMDEF_PM = 3,
    MT3_SKELETON_ANIM = 4,
    MT3_DETAIL_PATCH = 6,
    MT3_SKELETON_GEOMDEF_ST = 7,
    MT3_CACHED = 8,
    MT3_PARTICLE = 9,
    MT3_PROGRESSIVE2 = 10,
    MT3_LOD = 11,
    MT3_TREE = 12,
    MT3_SKELETON_RIGID = 15,
};

//----------------------------------------------------------------------------
//-- raw disk vertex formats for skinned geometry, byte-exact with
//-- OGSR-Engine's xr_3da/bone.h (vertBoned1W..4W)
//----------------------------------------------------------------------------
#pragma pack(push, 2)
struct ogf_vert_1w // 60 bytes (format_version == 4)
{
    float P[3], N[3], T[3], B[3];
    float u, v;
    ogf_u32 bone;
};
//-- format_version == 3 stores 1-link vertices WITHOUT tangent/binormal
//-- (2/3/4-link vertices are identical between format_version 3 and 4).
struct ogf_vert_1w_legacy // 36 bytes (format_version == 3)
{
    float P[3], N[3];
    float u, v;
    ogf_u32 bone;
};
struct ogf_vert_2w // 64 bytes
{
    ogf_u16 bone0, bone1;
    float P[3], N[3], T[3], B[3];
    float w; // blend weight of bone1 (bone0 gets 1-w)
    float u, v;
};
struct ogf_vert_3w // 70 bytes
{
    ogf_u16 bone[3];
    float P[3], N[3], T[3], B[3];
    float w[2]; // weights of bone[0], bone[1] (bone[2] gets 1-w0-w1)
    float u, v;
};
struct ogf_vert_4w // 76 bytes
{
    ogf_u16 bone[4];
    float P[3], N[3], T[3], B[3];
    float w[3]; // weights of bone[0..2] (bone[3] gets 1-w0-w1-w2)
    float u, v;
};
#pragma pack(pop)

//----------------------------------------------------------------------------
//-- minimal Direct3D FVF flags needed to decode a *static* (non-skinned)
//-- OGF_VERTICES chunk (MT_NORMAL meshes store plain D3DFVF-declared vertices,
//-- not the vertBonedXW layout above)
//----------------------------------------------------------------------------
constexpr ogf_u32 OGF_D3DFVF_XYZ = 0x002;
constexpr ogf_u32 OGF_D3DFVF_NORMAL = 0x010;
constexpr ogf_u32 OGF_D3DFVF_DIFFUSE = 0x040;
constexpr ogf_u32 OGF_D3DFVF_SPECULAR = 0x080;
constexpr ogf_u32 OGF_D3DFVF_TEXCOUNT_MASK = 0x0f00;
constexpr ogf_u32 OGF_D3DFVF_TEXCOUNT_SHIFT = 8;
