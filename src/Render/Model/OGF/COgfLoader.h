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
//--   motions / .omf (OGF_S_MOTIONS, OGF_S_MOTION_REFS[2])
//----------------------------------------------------------------------------
#pragma once
#include <string>
#include "COgfModel.h"

class COgfChunkedReader;

class COgfLoader
{
public:
    //-- Reads and parses `path` into `out`. Returns false on failure (see log).
    static bool Load(const std::string& path, SOgfModel& out);

private:
    static bool LoadVisual(COgfChunkedReader& r, SOgfModel& out, int depth);
    static bool LoadStaticGeometry(COgfChunkedReader& r, SOgfMeshDef& mesh, ogf_u32 verticesChunkId, ogf_u32 indicesChunkId);
    static bool LoadSkinnedGeometry(COgfChunkedReader& r, SOgfMeshDef& mesh, ogf_u8 formatVersion, ogf_u32 verticesChunkId, ogf_u32 indicesChunkId);
    static void LoadTexture(COgfChunkedReader& r, SOgfMeshDef& mesh);
    static bool LoadIndices(COgfChunkedReader& r, SOgfMeshDef& mesh, ogf_u32 indicesChunkId);
    static bool LoadSkeleton(COgfChunkedReader& r, SOgfModel& out, ogf_u8 formatVersion);
};
