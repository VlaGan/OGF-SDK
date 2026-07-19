//----------------------------------------------------------------------------
//-- COgfLoader.h
//-- Native .ogf file parser (no Assimp, no OGSR-Engine dependency).
//--
//-- Supports both known .ogf format versions:
//--   format_version == 4  - "release" format used by CS/CoP and OGSR-Engine
//--   format_version == 3  - Shadow of Chornobyl / pre-release build era
//--                          (different chunk ids, shorter header, and a
//--                          shorter 1-link skinned vertex struct - see
//--                          ogf_format.h for details)
//--
//-- Supported visual types (both format versions):
//--   MT_NORMAL / MT_PROGRESSIVE          - single static mesh
//--   MT_SKELETON_GEOMDEF_ST/_PM          - single skinned mesh (as a child)
//--   MT_SKELETON_RIGID                   - single skinned mesh (as a child)
//--   MT_HIERRARHY / MT_SKELETON_ANIM     - recurse into OGF_CHILDREN,
//--                                         MT_SKELETON_ANIM also loads the
//--                                         shared skeleton (OGF_S_BONE_NAMES
//--                                         + bind pose from whichever
//--                                         IK-data chunk variant is present)
//--
//-- Not implemented yet (logged and skipped, see README for the roadmap):
//--   MT_PROGRESSIVE fastpath / LOD sliding windows (OGF_SWIDATA, OGF_FASTPATH)
//--   MT_LOD, MT_TREE_*, MT_PARTICLE_*, MT_3DFLUIDVOLUME, MT3_DETAIL_PATCH, MT3_CACHED
//--
//-- Motions (.omf) ARE supported via LoadMotions(): OGF_S_MOTION_REFS/2 (or
//-- OGF3_S_MOTION_REFS) list referenced .omf file names, each of which is
//-- loaded separately and decoded (OGF_S_SMPARAMS for the bone-name table,
//-- OGF_S_MOTIONS for the actual compressed keyframes) into ready-to-use
//-- CMotion objects. See OGSR-Engine's xr_3da/SkeletonMotions.cpp
//-- (motions_value::load) and Layers/xrRender/AnimationKeyCalculate.h
//-- (key dequantization) for the reference implementation this was ported
//-- from, and OGF-tool's Classes/OGF Chunks/*.cs for the chunk id variants
//-- across format_version 3 sub-builds.
//----------------------------------------------------------------------------
#pragma once
#include <string>
#include "COgfModel.h"

class COgfChunkedReader;

class COgfLoader
{
public:
    //-- Reads and parses `path` into `out`. Returns false on failure (see log).
    //-- Only reads the .ogf itself (geometry/skeleton/motion *references*);
    //-- call LoadMotions() afterwards to actually resolve+load .omf files.
    static bool Load(const std::string& path, SOgfModel& out);

    //-- Resolves every name in out.motionRefs to "<dir of ogfPath>/<name>.omf",
    //-- loads and decodes each one, and appends the result to out.motions.
    //-- Returns true if at least one motion was loaded. Safe to call even if
    //-- out.motionRefs is empty (returns false, no-op).
    static bool LoadMotions(const std::string& ogfPath, SOgfModel& out);

private:
    static bool LoadVisual(COgfChunkedReader& r, SOgfModel& out, int depth);
    static bool LoadStaticGeometry(COgfChunkedReader& r, SOgfMeshDef& mesh, ogf_u32 verticesChunkId, ogf_u32 indicesChunkId);
    static bool LoadSkinnedGeometry(COgfChunkedReader& r, SOgfMeshDef& mesh, ogf_u8 formatVersion, ogf_u32 verticesChunkId, ogf_u32 indicesChunkId);
    static void LoadTexture(COgfChunkedReader& r, SOgfMeshDef& mesh);
    static bool LoadIndices(COgfChunkedReader& r, SOgfMeshDef& mesh, ogf_u32 indicesChunkId);
    static bool LoadSkeleton(COgfChunkedReader& r, SOgfModel& out, ogf_u8 formatVersion);
    static bool LoadMotionRefs(COgfChunkedReader& r, ogf_u8 formatVersion, SOgfModel& out);

    //-- shared by embedded (rare) and external .omf motion data:
    //-- parses OGF_S_SMPARAMS (only for its bone-name table) + OGF_S_MOTIONS
    //-- from `root` and appends decoded CMotion objects to `outMotions`.
    //-- `logIfMissing=false` silences the "no SMPARAMS/MOTIONS chunk" messages
    //-- for speculative calls where absence is the normal case, not an error.
    //-- `formatVersionHint`: -1 (default) = unknown source (a standalone .omf
    //-- file has no format_version byte of its own) - try every known chunk
    //-- id variant. When the caller DOES know the exact format_version (i.e.
    //-- reading straight from the .ogf's own top-level stream), pass it here
    //-- to restrict the search to that version's ids only - critical because
    //-- OGF3_S_SMPARAMS(18) collides with the unrelated OGF_S_DESC(18) chunk
    //-- id used by format_version 4, and blindly trying it against a v4 file
    //-- misreads OGF_S_DESC's text as SMPARAMS binary data (garbage bone
    //-- counts/slots -> a multi-gigabyte vector resize -> std::bad_alloc).
    static bool LoadSMParamsAndMotions(COgfChunkedReader& root, std::vector<CMotion>& outMotions, const std::string& debugName, bool logIfMissing = true, int formatVersionHint = -1);
    static bool LoadMotionsFile(const std::string& path, std::vector<CMotion>& outMotions);
};
