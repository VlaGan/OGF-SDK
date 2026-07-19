//----------------------------------------------------------------------------
//-- COgfLoader.cpp
//----------------------------------------------------------------------------
#include "COgfLoader.h"
#include "COgfChunkedReader.h"

#include <fstream>
#include <cstring>

#include "../../../_defines.h" // pulls in Core/CLog.h (LogMsg)

//----------------------------------------------------------------------------
//-- format_version dependent chunk id / classification helpers
//-- (format_version == 4: "release" ids from ogf_format.h's EOgfChunk;
//--  format_version == 3 and older: EOgfChunkV3 - see ogf_format.h)
//----------------------------------------------------------------------------
namespace
{
    enum class EOgfKind
    {
        Normal, // static single mesh (also covers MT_PROGRESSIVE - same layout, LOD data just ignored)
        Hierarchy, // no skeleton, just OGF_CHILDREN of (usually) static meshes
        SkeletonAnim, // OGF_CHILDREN of skinned meshes + OGF_S_BONE_NAMES
        SkeletonGeomdefST,
        SkeletonGeomdefPM,
        SkeletonRigid,
        Unsupported,
    };

    EOgfKind ClassifyOgfType(ogf_u8 formatVersion, ogf_u8 type)
    {
        if (formatVersion == OGF_FORMAT_VERSION) // 4
        {
            switch (type)
            {
            case MT_NORMAL:
            case MT_PROGRESSIVE: return EOgfKind::Normal;
            case MT_HIERRARHY: return EOgfKind::Hierarchy;
            case MT_SKELETON_ANIM: return EOgfKind::SkeletonAnim;
            case MT_SKELETON_GEOMDEF_ST: return EOgfKind::SkeletonGeomdefST;
            case MT_SKELETON_GEOMDEF_PM: return EOgfKind::SkeletonGeomdefPM;
            case MT_SKELETON_RIGID: return EOgfKind::SkeletonRigid;
            default: return EOgfKind::Unsupported;
            }
        }

        //-- format_version 3 (and best-effort fallback for older/unknown versions)
        switch (type)
        {
        case MT3_NORMAL:
        case MT3_PROGRESSIVE:
        case MT3_PROGRESSIVE2: return EOgfKind::Normal;
        case MT3_HIERRARHY: return EOgfKind::Hierarchy;
        case MT3_SKELETON_ANIM: return EOgfKind::SkeletonAnim;
        case MT3_SKELETON_GEOMDEF_ST: return EOgfKind::SkeletonGeomdefST;
        case MT3_SKELETON_GEOMDEF_PM: return EOgfKind::SkeletonGeomdefPM;
        case MT3_SKELETON_RIGID: return EOgfKind::SkeletonRigid;
        default: return EOgfKind::Unsupported;
        }
    }

    ogf_u32 VerticesChunkId(ogf_u8 formatVersion) { return formatVersion == OGF_FORMAT_VERSION ? (ogf_u32)OGF_VERTICES : (ogf_u32)OGF3_VERTICES; }
    ogf_u32 IndicesChunkId(ogf_u8 formatVersion) { return formatVersion == OGF_FORMAT_VERSION ? (ogf_u32)OGF_INDICES : (ogf_u32)OGF3_INDICES; }
    ogf_u32 ChildrenChunkId(ogf_u8 formatVersion) { return formatVersion == OGF_FORMAT_VERSION ? (ogf_u32)OGF_CHILDREN : (ogf_u32)OGF3_CHILDREN; }
} // namespace

//----------------------------------------------------------------------------
bool COgfLoader::Load(const std::string& path, SOgfModel& out)
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
        LogMsg("!COgfLoader::Load: cant open file [%s]", path.c_str());
        return false;
    }

    const std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    if (fileSize <= 0)
    {
        LogMsg("!COgfLoader::Load: file [%s] is empty", path.c_str());
        return false;
    }

    std::vector<uint8_t> buffer(static_cast<size_t>(fileSize));
    if (!file.read(reinterpret_cast<char*>(buffer.data()), fileSize))
    {
        LogMsg("!COgfLoader::Load: failed reading file [%s]", path.c_str());
        return false;
    }

    out = SOgfModel{};
    out.sourcePath = path;

    //-- `buffer` stays alive for the whole synchronous parse below; every
    //-- value we need is copied out into `out` before this function returns.
    COgfChunkedReader root(buffer.data(), buffer.size());
    const bool ok = LoadVisual(root, out, 0);

    if (ok)
    {
        LogMsg("-COgfLoader::Load: [%s] (format v%u) -> %zu mesh(es), %zu bone(s)",
            path.c_str(), out.formatVersion, out.meshes.size(), out.bones.size());
    }

    return ok;
}

//----------------------------------------------------------------------------
//-- reads OGF_TEXTURE (texture + shader name), if present - chunk id 2 is
//-- shared between format_version 3 and 4
//----------------------------------------------------------------------------
void COgfLoader::LoadTexture(COgfChunkedReader& r, SOgfMeshDef& mesh)
{
    COgfChunkedReader tex;
    if (r.open_chunk(OGF_TEXTURE, tex))
    {
        mesh.textureName = tex.r_stringZ();
        mesh.shaderName = tex.r_stringZ();
    }
}

//----------------------------------------------------------------------------
//-- reads OGF_INDICES: u32 count, then count * u16 indices
//----------------------------------------------------------------------------
bool COgfLoader::LoadIndices(COgfChunkedReader& r, SOgfMeshDef& mesh, ogf_u32 indicesChunkId)
{
    COgfChunkedReader idx;
    if (!r.open_chunk(indicesChunkId, idx))
    {
        LogMsg("!COgfLoader: missing indices chunk [%u]", indicesChunkId);
        return false;
    }

    const ogf_u32 idxCount = idx.r_u32();
    if (idx.remaining() < static_cast<size_t>(idxCount) * sizeof(uint16_t))
    {
        LogMsg("!COgfLoader: indices chunk truncated");
        return false;
    }

    const uint16_t* idxPtr = reinterpret_cast<const uint16_t*>(idx.pointer());
    mesh.indices.reserve(idxCount);
    for (ogf_u32 i = 0; i < idxCount; ++i)
        mesh.indices.push_back(idxPtr[i]);

    return true;
}

//----------------------------------------------------------------------------
//-- OGF_VERTICES for a *static* (MT_NORMAL / MT_PROGRESSIVE) mesh: a plain
//-- D3DFVF-declared vertex stream. We decode the common case (position +
//-- normal + 1 UV channel); extra channels (2nd UV, vertex colors) are
//-- skipped. Layout is identical between format_version 3 and 4.
//----------------------------------------------------------------------------
bool COgfLoader::LoadStaticGeometry(COgfChunkedReader& r, SOgfMeshDef& mesh, ogf_u32 verticesChunkId, ogf_u32 indicesChunkId)
{
    COgfChunkedReader verts;
    if (!r.open_chunk(verticesChunkId, verts))
    {
        LogMsg("!COgfLoader: missing vertices chunk [%u]", verticesChunkId);
        return false;
    }

    const ogf_u32 fvf = verts.r_u32();
    const ogf_u32 vertCount = verts.r_u32();

    size_t offsetPos = SIZE_MAX, offsetNormal = SIZE_MAX, offsetUV = SIZE_MAX;
    size_t stride = 0;

    if (fvf & OGF_D3DFVF_XYZ)
    {
        offsetPos = stride;
        stride += 12;
    }
    if (fvf & OGF_D3DFVF_NORMAL)
    {
        offsetNormal = stride;
        stride += 12;
    }
    if (fvf & OGF_D3DFVF_DIFFUSE)
        stride += 4;
    if (fvf & OGF_D3DFVF_SPECULAR)
        stride += 4;

    const ogf_u32 texCount = (fvf & OGF_D3DFVF_TEXCOUNT_MASK) >> OGF_D3DFVF_TEXCOUNT_SHIFT;
    if (texCount >= 1)
    {
        offsetUV = stride;
        stride += 8;
    }
    for (ogf_u32 t = 1; t < texCount; ++t)
        stride += 8; // extra channels assumed 2D, skipped

    if (offsetPos == SIZE_MAX || stride == 0)
    {
        LogMsg("!COgfLoader: unsupported static vertex FVF [0x%X]", fvf);
        return false;
    }

    if (verts.remaining() < static_cast<size_t>(vertCount) * stride)
    {
        LogMsg("!COgfLoader: OGF_VERTICES chunk truncated");
        return false;
    }

    const uint8_t* base = verts.pointer();
    mesh.vertices.reserve(vertCount);
    for (ogf_u32 i = 0; i < vertCount; ++i)
    {
        const uint8_t* v = base + static_cast<size_t>(i) * stride;

        Vertex out{};
        float p[3];
        std::memcpy(p, v + offsetPos, sizeof(p));
        out.position = {p[0], p[1], p[2]};

        if (offsetNormal != SIZE_MAX)
        {
            float n[3];
            std::memcpy(n, v + offsetNormal, sizeof(n));
            out.normal = {n[0], n[1], n[2]};
        }

        if (offsetUV != SIZE_MAX)
        {
            float uv[2];
            std::memcpy(uv, v + offsetUV, sizeof(uv));
            out.texcoord = {uv[0], uv[1]};
        }

        mesh.vertices.push_back(out);
    }

    return LoadIndices(r, mesh, indicesChunkId);
}

//----------------------------------------------------------------------------
//-- OGF_VERTICES for a skinned mesh: dwVertType selects one of the four
//-- vertBonedXW raw layouts (ogf_format.h). Bone weight semantics match
//-- OGSR-Engine's FSkinned.cpp get_pos_bones()/get_weightN() helpers.
//--
//-- IMPORTANT format_version difference (ported from OGF-tool/Children.cs):
//-- the single-link (1L) case has NO tangent/binormal when format_version==3
//-- (36 bytes, ogf_vert_1w_legacy) vs format_version==4 (60 bytes, with T/B,
//-- ogf_vert_1w). 2/3/4-link vertices are identical between both versions.
//----------------------------------------------------------------------------
bool COgfLoader::LoadSkinnedGeometry(COgfChunkedReader& r, SOgfMeshDef& mesh, ogf_u8 formatVersion, ogf_u32 verticesChunkId, ogf_u32 indicesChunkId)
{
    COgfChunkedReader verts;
    if (!r.open_chunk(verticesChunkId, verts))
    {
        LogMsg("!COgfLoader: missing vertices chunk [%u]", verticesChunkId);
        return false;
    }

    const ogf_u32 vertType = verts.r_u32();
    const ogf_u32 vertCount = verts.r_u32();
    const uint8_t* base = verts.pointer();

    auto setBone = [](Vertex& v, int slot, int boneId, float weight) {
        if (slot >= 0 && slot < MAX_BONE_INFLUENCE)
        {
            v.boneIDs[slot] = boneId;
            v.weights[slot] = weight;
        }
    };

    mesh.isSkinned = true;
    mesh.vertices.resize(vertCount);

    switch (vertType)
    {
    case OGF_VERTEXFORMAT_FVF_1L:
    case 1:
    {
        if (formatVersion == OGF_FORMAT_VERSION)
        {
            if (verts.remaining() < static_cast<size_t>(vertCount) * sizeof(ogf_vert_1w))
            {
                LogMsg("!COgfLoader: OGF_VERTICES (1L) truncated");
                return false;
            }
            const ogf_vert_1w* src = reinterpret_cast<const ogf_vert_1w*>(base);
            for (ogf_u32 i = 0; i < vertCount; ++i)
            {
                const ogf_vert_1w& s = src[i];
                Vertex& v = mesh.vertices[i];
                v.position = {s.P[0], s.P[1], s.P[2]};
                v.normal = {s.N[0], s.N[1], s.N[2]};
                v.texcoord = {s.u, s.v};
                setBone(v, 0, static_cast<int>(s.bone), 1.0f);
            }
        }
        else // format_version 3: no tangent/binormal in the 1-link struct
        {
            if (verts.remaining() < static_cast<size_t>(vertCount) * sizeof(ogf_vert_1w_legacy))
            {
                LogMsg("!COgfLoader: OGF_VERTICES (1L, legacy) truncated");
                return false;
            }
            const ogf_vert_1w_legacy* src = reinterpret_cast<const ogf_vert_1w_legacy*>(base);
            for (ogf_u32 i = 0; i < vertCount; ++i)
            {
                const ogf_vert_1w_legacy& s = src[i];
                Vertex& v = mesh.vertices[i];
                v.position = {s.P[0], s.P[1], s.P[2]};
                v.normal = {s.N[0], s.N[1], s.N[2]};
                v.texcoord = {s.u, s.v};
                setBone(v, 0, static_cast<int>(s.bone), 1.0f);
            }
        }
        break;
    }
    case OGF_VERTEXFORMAT_FVF_2L:
    case 2:
    {
        if (verts.remaining() < static_cast<size_t>(vertCount) * sizeof(ogf_vert_2w))
        {
            LogMsg("!COgfLoader: OGF_VERTICES (2L) truncated");
            return false;
        }
        const ogf_vert_2w* src = reinterpret_cast<const ogf_vert_2w*>(base);
        for (ogf_u32 i = 0; i < vertCount; ++i)
        {
            const ogf_vert_2w& s = src[i];
            Vertex& v = mesh.vertices[i];
            v.position = {s.P[0], s.P[1], s.P[2]};
            v.normal = {s.N[0], s.N[1], s.N[2]};
            v.texcoord = {s.u, s.v};
            setBone(v, 0, s.bone0, 1.0f - s.w);
            setBone(v, 1, s.bone1, s.w);
        }
        break;
    }
    case OGF_VERTEXFORMAT_FVF_3L:
    case 3:
    {
        if (verts.remaining() < static_cast<size_t>(vertCount) * sizeof(ogf_vert_3w))
        {
            LogMsg("!COgfLoader: OGF_VERTICES (3L) truncated");
            return false;
        }
        const ogf_vert_3w* src = reinterpret_cast<const ogf_vert_3w*>(base);
        for (ogf_u32 i = 0; i < vertCount; ++i)
        {
            const ogf_vert_3w& s = src[i];
            Vertex& v = mesh.vertices[i];
            v.position = {s.P[0], s.P[1], s.P[2]};
            v.normal = {s.N[0], s.N[1], s.N[2]};
            v.texcoord = {s.u, s.v};
            setBone(v, 0, s.bone[0], s.w[0]);
            setBone(v, 1, s.bone[1], s.w[1]);
            setBone(v, 2, s.bone[2], 1.0f - s.w[0] - s.w[1]);
        }
        break;
    }
    case OGF_VERTEXFORMAT_FVF_4L:
    case 4:
    {
        if (verts.remaining() < static_cast<size_t>(vertCount) * sizeof(ogf_vert_4w))
        {
            LogMsg("!COgfLoader: OGF_VERTICES (4L) truncated");
            return false;
        }
        const ogf_vert_4w* src = reinterpret_cast<const ogf_vert_4w*>(base);
        for (ogf_u32 i = 0; i < vertCount; ++i)
        {
            const ogf_vert_4w& s = src[i];
            Vertex& v = mesh.vertices[i];
            v.position = {s.P[0], s.P[1], s.P[2]};
            v.normal = {s.N[0], s.N[1], s.N[2]};
            v.texcoord = {s.u, s.v};
            setBone(v, 0, s.bone[0], s.w[0]);
            setBone(v, 1, s.bone[1], s.w[1]);
            setBone(v, 2, s.bone[2], s.w[2]);
            setBone(v, 3, s.bone[3], 1.0f - s.w[0] - s.w[1] - s.w[2]);
        }
        break;
    }
    default:
        LogMsg("!COgfLoader: unsupported skinned vertex type [0x%X]", vertType);
        return false;
    }

    return LoadIndices(r, mesh, indicesChunkId);
}

//----------------------------------------------------------------------------
//-- OGF_S_BONE_NAMES (name, parent name, obb - obb currently unused, chunk id
//-- 13 is shared between format_version 3 and 4) and the bind-pose
//-- rotation/position, which lives in one of three possible IK-data chunk
//-- variants depending on the exact build the file was exported from. Ported
//-- from OGF-tool/IKData.cs (IK_Data.Load, chunk_version 2/3/4):
//--
//--   chunk_version 4 (OGF4_S_IKDATA, or OGF3_S_IKDATA_2 for format_version 3):
//--       per-bone: u32 version, then material name, SBoneShape(112 bytes),
//--       then a 76-byte "Import" block (includes friction if per-bone
//--       version > 0 - see OGSR-Engine SkeletonCustom.cpp)
//--   chunk_version 3 (OGF3_S_IKDATA, format_version 3 only):
//--       no per-bone version field; Import block is 72 bytes (no friction)
//--   chunk_version 2 (OGF3_S_IKDATA_0, format_version 3 only, oldest):
//--       no per-bone version field; Import block is 60 bytes (no ik_flags/
//--       break_force/break_torque either)
//--
//-- In every variant, rotation/position/mass/center_of_mass follow the
//-- Import block in the same order.
//----------------------------------------------------------------------------
bool COgfLoader::LoadSkeleton(COgfChunkedReader& r, SOgfModel& out, ogf_u8 formatVersion)
{
    COgfChunkedReader names;
    if (!r.open_chunk(OGF_S_BONE_NAMES, names))
        return false;

    const ogf_u32 count = names.r_u32();
    out.bones.reserve(out.bones.size() + count);

    std::vector<std::string> parentNames;
    parentNames.reserve(count);

    for (ogf_u32 i = 0; i < count; ++i)
    {
        SOgfBoneDef bone;
        bone.name = names.r_stringZ();
        parentNames.push_back(names.r_stringZ());
        names.skip(60); // Fobb: 3x3 rotation (36) + translate(12) + halfsize(12) - unused for now
        out.bones.push_back(std::move(bone));
    }

    //-- resolve which IK-data chunk/sub-version is present (see comment above)
    const ogf_u32 ikChunkRelease = (formatVersion == OGF_FORMAT_VERSION) ? (ogf_u32)OGF_S_IKDATA : (ogf_u32)OGF3_S_IKDATA_2;

    COgfChunkedReader ikData;
    int ikVersion = 0; // 4, 3, or 2 - see comment above; 0 = not found
    if (r.open_chunk(ikChunkRelease, ikData))
        ikVersion = 4;
    else if (formatVersion != OGF_FORMAT_VERSION && r.open_chunk(OGF3_S_IKDATA, ikData))
        ikVersion = 3;
    else if (formatVersion != OGF_FORMAT_VERSION && r.open_chunk(OGF3_S_IKDATA_0, ikData))
        ikVersion = 2;

    if (ikVersion != 0)
    {
        for (ogf_u32 i = 0; i < count; ++i)
        {
            if (ikVersion == 4)
            {
                const ogf_u16 vers = static_cast<ogf_u16>(ikData.r_u32());

                ikData.r_stringZ(); // game material name - unused for now
                ikData.skip(112); // SBoneShape - unused for now

                // SJointIKData::Import (76 bytes total, or 72 if this bone's own `vers` is 0)
                ikData.skip(4); // joint type
                ikData.skip(3 * 16); // 3x SJointLimit { Fvector2 limit; float spring; float damping; }
                ikData.skip(4 + 4); // spring_factor, damping_factor
                ikData.skip(4); // ik_flags
                ikData.skip(4 + 4); // break_force, break_torque
                if (vers > 0)
                    ikData.skip(4); // friction
            }
            else
            {
                ikData.r_stringZ(); // game material name
                ikData.skip(112); // SBoneShape

                if (ikVersion == 3)
                {
                    // Import (72 bytes): type + 3x limit + spring + damping + ik_flags + break_force/torque
                    ikData.skip(4 + 3 * 16 + 4 + 4 + 4 + 4 + 4);
                }
                else // ikVersion == 2, oldest: no ik_flags/break_force/break_torque
                {
                    // Import (60 bytes): type + 3x limit + spring + damping
                    ikData.skip(4 + 3 * 16 + 4 + 4);
                }
            }

            float rot[3], pos[3];
            ikData.r_fvector3(rot);
            ikData.r_fvector3(pos);

            const size_t boneIdx = out.bones.size() - count + i;
            out.bones[boneIdx].rotation = {rot[0], rot[1], rot[2]};
            out.bones[boneIdx].position = {pos[0], pos[1], pos[2]};

            ikData.skip(4); // mass
            ikData.skip(12); // center_of_mass
        }
    }
    else
    {
        LogMsg("~COgfLoader: model has no recognizable IK-data chunk, bones will use identity bind pose");
    }

    //-- resolve parent name -> parent index
    const size_t baseIdx = out.bones.size() - count;
    for (ogf_u32 i = 0; i < count; ++i)
    {
        SOgfBoneDef& bone = out.bones[baseIdx + i];
        bone.parentIndex = -1;
        if (parentNames[i].empty())
            continue;

        for (ogf_u32 j = 0; j < count; ++j)
        {
            if (out.bones[baseIdx + j].name == parentNames[i])
            {
                bone.parentIndex = static_cast<int>(baseIdx + j);
                break;
            }
        }
    }

    return true;
}

//----------------------------------------------------------------------------
//-- OGF_S_MOTION_REFS: a single comma-separated stringZ list of .omf base
//-- names (no path/extension). OGF_S_MOTION_REFS2: u32 count + that many
//-- stringZ names directly. format_version 3 only ever uses the REFS variant
//-- (chunk id 29). Ported from OGSR-Engine's SkeletonAnimated.cpp
//-- (CKinematicsAnimated::Load) - wildcard "*.omf" directory listing isn't
//-- supported here (that needs a filesystem scan, not just chunk parsing).
//----------------------------------------------------------------------------
bool COgfLoader::LoadMotionRefs(COgfChunkedReader& r, ogf_u8 formatVersion, SOgfModel& out)
{
    COgfChunkedReader refs;
    const ogf_u32 refsId = (formatVersion == OGF_FORMAT_VERSION) ? (ogf_u32)OGF_S_MOTION_REFS : (ogf_u32)OGF3_S_MOTION_REFS;

    if (r.open_chunk(refsId, refs))
    {
        const std::string items = refs.r_stringZ();
        size_t start = 0;
        while (start <= items.size())
        {
            const size_t comma = items.find(',', start);
            const std::string item = (comma == std::string::npos) ? items.substr(start) : items.substr(start, comma - start);
            if (!item.empty())
                out.motionRefs.push_back(item);
            if (comma == std::string::npos)
                break;
            start = comma + 1;
        }
        return !out.motionRefs.empty();
    }

    if (formatVersion == OGF_FORMAT_VERSION && r.open_chunk(OGF_S_MOTION_REFS2, refs))
    {
        const ogf_u32 count = refs.r_u32();
        for (ogf_u32 i = 0; i < count; ++i)
            out.motionRefs.push_back(refs.r_stringZ());
        return !out.motionRefs.empty();
    }

    return false;
}

//----------------------------------------------------------------------------
//-- OGF_S_SMPARAMS + OGF_S_MOTIONS decoding. Both an .omf file's top level
//-- AND (in principle) an .ogf's own top level use this exact same chunk
//-- pair/layout, so this one routine serves either case.
//--
//-- Ported from OGSR-Engine's xr_3da/SkeletonMotions.cpp (motions_value::load)
//-- for the chunk layout, and Layers/xrRender/AnimationKeyCalculate.h for the
//-- key dequantization math (QR2Quat / QT8_2T / QT16_2T).
//--
//-- We only need OGF_S_SMPARAMS for its "motion storage slot -> bone name"
//-- table (built from each partition's bone list) - the rest of SMPARAMS
//-- (motion defs: speed/power/accrue/falloff/marks) is blend-runtime-only
//-- metadata that a preview/editor doesn't need, so it's simply never read.
//----------------------------------------------------------------------------
bool COgfLoader::LoadSMParamsAndMotions(COgfChunkedReader& root, std::vector<CMotion>& outMotions, const std::string& debugName, bool logIfMissing, int formatVersionHint)
{
    //-- restrict candidate chunk ids to the known format_version when the
    //-- caller tells us it (see header comment - avoids the OGF3_S_SMPARAMS(18)
    //-- / OGF_S_DESC(18) collision on v4 files)
    const bool tryV4 = (formatVersionHint < 0 || formatVersionHint == OGF_FORMAT_VERSION);
    const bool tryV3 = (formatVersionHint < 0 || formatVersionHint != OGF_FORMAT_VERSION);

    COgfChunkedReader sm;
    bool haveSM = false;
    if (tryV4)
        haveSM = root.open_chunk(OGF_S_SMPARAMS, sm);
    if (!haveSM && tryV3)
        haveSM = root.open_chunk(OGF3_S_SMPARAMS_NEW, sm) || root.open_chunk(OGF3_S_SMPARAMS, sm);

    if (!haveSM)
    {
        if (logIfMissing)
            LogMsg("!COgfLoader::LoadMotions: no SMPARAMS chunk in [%s]", debugName.c_str());
        return false;
    }

    sm.r_u16(); // version - only affects the CMotionDef marks section we don't read
    const ogf_u16 partCount = sm.r_u16();

    //-- sanity bound: a real SMPARAMS never has anywhere near this many
    //-- partitions/bones. Guards against ever mis-parsing an unrelated chunk
    //-- (wrong id, or garbage) as SMPARAMS and trying to allocate gigabytes.
    constexpr ogf_u16 kMaxReasonableCount = 4096;

    std::vector<std::string> slotToBoneName;
    for (ogf_u16 p = 0; p < partCount && p < kMaxReasonableCount; ++p)
    {
        sm.r_stringZ(); // partition name - unused
        const ogf_u16 boneCount = sm.r_u16();
        for (ogf_u16 b = 0; b < boneCount && b < kMaxReasonableCount; ++b)
        {
            std::string boneName = sm.r_stringZ();
            const ogf_u32 slot = sm.r_u32();
            if (slot >= kMaxReasonableCount)
            {
                if (logIfMissing)
                    LogMsg("!COgfLoader::LoadMotions: implausible bone slot [%u] in [%s], SMPARAMS is probably a misidentified chunk - aborting", slot, debugName.c_str());
                return false;
            }
            if (slotToBoneName.size() <= slot)
                slotToBoneName.resize(slot + 1);
            slotToBoneName[slot] = std::move(boneName);
        }
    }

    if (slotToBoneName.empty())
    {
        if (logIfMissing)
            LogMsg("!COgfLoader::LoadMotions: SMPARAMS has no bones in [%s]", debugName.c_str());
        return false;
    }

    COgfChunkedReader ms;
    bool haveMS = false;
    if (tryV4)
        haveMS = root.open_chunk(OGF_S_MOTIONS, ms);
    if (!haveMS && tryV3)
        haveMS = root.open_chunk(OGF3_S_MOTIONS_NEW, ms);

    if (!haveMS)
    {
        if (logIfMissing)
            LogMsg("!COgfLoader::LoadMotions: no MOTIONS chunk in [%s]", debugName.c_str());
        return false;
    }

    COgfChunkedReader countChunk;
    if (!ms.open_chunk(0, countChunk))
    {
        if (logIfMissing)
            LogMsg("!COgfLoader::LoadMotions: MOTIONS chunk missing motion count in [%s]", debugName.c_str());
        return false;
    }
    const ogf_u32 motionCount = countChunk.r_u32();

    //-- same defense-in-depth bound as above, for the same reason
    constexpr ogf_u32 kMaxReasonableMotions = 200000;
    if (motionCount > kMaxReasonableMotions)
    {
        if (logIfMissing)
            LogMsg("!COgfLoader::LoadMotions: implausible motion count [%u] in [%s], MOTIONS is probably a misidentified chunk - aborting", motionCount, debugName.c_str());
        return false;
    }

    outMotions.reserve(outMotions.size() + motionCount);

    for (ogf_u32 m = 0; m < motionCount; ++m)
    {
        COgfChunkedReader mot;
        if (!ms.open_chunk(m + 1, mot)) // motions are numbered starting at 1, slot 0 is the count above
        {
            LogMsg("~COgfLoader::LoadMotions: motion #%u missing in [%s]", m, debugName.c_str());
            continue;
        }

        const std::string motionName = mot.r_stringZ();
        const ogf_u32 frameCount = mot.r_u32();

        CMotion motion(motionName, frameCount > 0 ? float(frameCount) : 1.0f, OGF_SAMPLE_FPS);

        for (size_t slot = 0; slot < slotToBoneName.size(); ++slot)
        {
            BoneMotionData bmd{};
            bmd.name = slotToBoneName[slot];

            const ogf_u8 flags = mot.r_u8();

            //-- rotation: either one constant key, or `frameCount` keyframes
            if (flags & OGF_MOTION_FL_R_KEY_ABSENT)
            {
                ogf_key_qr k{};
                mot.r(&k, sizeof(k));
                bmd.rotations.push_back({{float(k.x) * OGF_KEY_QUANT_I, float(k.y) * OGF_KEY_QUANT_I, float(k.z) * OGF_KEY_QUANT_I, float(k.w) * OGF_KEY_QUANT_I}, 0.f});
            }
            else
            {
                mot.r_u32(); // crc - unused
                bmd.rotations.reserve(frameCount);
                for (ogf_u32 f = 0; f < frameCount; ++f)
                {
                    ogf_key_qr k{};
                    mot.r(&k, sizeof(k));
                    bmd.rotations.push_back({{float(k.x) * OGF_KEY_QUANT_I, float(k.y) * OGF_KEY_QUANT_I, float(k.z) * OGF_KEY_QUANT_I, float(k.w) * OGF_KEY_QUANT_I}, float(f)});
                }
            }

            //-- translation: either `frameCount` quantized keyframes (8 or 16
            //-- bit, dequantized as raw*sizeT+initT), or one constant vector
            if (flags & OGF_MOTION_FL_T_KEY_PRESENT)
            {
                mot.r_u32(); // crc - unused

                std::vector<DirectX::XMFLOAT3> raw;
                raw.reserve(frameCount);
                if (flags & OGF_MOTION_FL_T_KEY_16BIT)
                {
                    for (ogf_u32 f = 0; f < frameCount; ++f)
                    {
                        ogf_key_qt16 k{};
                        mot.r(&k, sizeof(k));
                        raw.push_back({float(k.x), float(k.y), float(k.z)});
                    }
                }
                else
                {
                    for (ogf_u32 f = 0; f < frameCount; ++f)
                    {
                        ogf_key_qt8 k{};
                        mot.r(&k, sizeof(k));
                        raw.push_back({float(k.x), float(k.y), float(k.z)});
                    }
                }

                float sizeT[3], initT[3];
                mot.r_fvector3(sizeT);
                mot.r_fvector3(initT);

                bmd.positions.reserve(frameCount);
                for (ogf_u32 f = 0; f < frameCount; ++f)
                {
                    bmd.positions.push_back({{raw[f].x * sizeT[0] + initT[0], raw[f].y * sizeT[1] + initT[1], raw[f].z * sizeT[2] + initT[2]}, float(f)});
                }
            }
            else
            {
                float initT[3];
                mot.r_fvector3(initT);
                bmd.positions.push_back({{initT[0], initT[1], initT[2]}, 0.f});
            }

            //-- X-Ray never animates per-bone scale; CModel::InterpolateScaling
            //-- (and Position/Rotation) index scales[i+1] without checking for
            //-- an empty vector, so always provide exactly one identity key.
            if (bmd.positions.empty())
                bmd.positions.push_back({{0.f, 0.f, 0.f}, 0.f});
            if (bmd.rotations.empty())
                bmd.rotations.push_back({{0.f, 0.f, 0.f, 1.f}, 0.f});
            bmd.scales.push_back({{1.f, 1.f, 1.f}, 0.f});

            motion.AddBoneMtData(bmd);
        }

        outMotions.push_back(std::move(motion));
    }

    return true;
}

//----------------------------------------------------------------------------
bool COgfLoader::LoadMotionsFile(const std::string& path, std::vector<CMotion>& outMotions)
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
        LogMsg("!COgfLoader::LoadMotionsFile: cant open [%s]", path.c_str());
        return false;
    }

    const std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    if (fileSize <= 0)
        return false;

    std::vector<uint8_t> buffer(static_cast<size_t>(fileSize));
    if (!file.read(reinterpret_cast<char*>(buffer.data()), fileSize))
        return false;

    //-- an .omf file has no OGF_HEADER of its own - it's just SMPARAMS +
    //-- MOTIONS sitting directly at the top level
    COgfChunkedReader root(buffer.data(), buffer.size());
    return LoadSMParamsAndMotions(root, outMotions, path);
}

//----------------------------------------------------------------------------
bool COgfLoader::LoadMotions(const std::string& ogfPath, SOgfModel& out)
{
    if (out.motionRefs.empty())
        return false;

    const size_t slash = ogfPath.find_last_of("/\\");
    const std::string dir = (slash == std::string::npos) ? std::string() : ogfPath.substr(0, slash + 1);

    bool any = false;
    for (const auto& ref : out.motionRefs)
    {
        const std::string omfPath = dir + ref + ".omf";

        std::vector<CMotion> loaded;
        if (LoadMotionsFile(omfPath, loaded))
        {
            LogMsg("-COgfLoader::LoadMotions: [%s] -> %zu motion(s)", omfPath.c_str(), loaded.size());
            for (auto& m : loaded)
                out.motions.push_back(std::move(m));
            any = true;
        }
        else
        {
            LogMsg("~COgfLoader::LoadMotions: could not load referenced motion file [%s]", omfPath.c_str());
        }
    }

    return any;
}

//----------------------------------------------------------------------------
//-- parses one visual (recursively, for hierarchical/skeletal models)
//----------------------------------------------------------------------------
bool COgfLoader::LoadVisual(COgfChunkedReader& r, SOgfModel& out, int depth)
{
    COgfChunkedReader header;
    if (!r.open_chunk(OGF_HEADER, header))
    {
        LogMsg("!COgfLoader: missing OGF_HEADER chunk (depth %d)", depth);
        return false;
    }

    const ogf_u8 formatVersion = header.r_u8();
    const ogf_u8 type = header.r_u8();
    header.r_u16(); // shader_id - only meaningful for compiled level geometry, unused here

    //-- IMPORTANT: format_version 3 headers are only 4 bytes (no bbox/bsphere)!
    //-- Only format_version 4 stores a bounding box/sphere right in the header.
    float bboxMin[3] = {0, 0, 0}, bboxMax[3] = {0, 0, 0};
    if (formatVersion == OGF_FORMAT_VERSION)
    {
        header.r_fvector3(bboxMin);
        header.r_fvector3(bboxMax);
        // bsphere (center + radius) follows in the stream but isn't needed here
    }

    if (formatVersion != 3 && formatVersion != OGF_FORMAT_VERSION)
        LogMsg("~COgfLoader: untested ogf format version [%u] (known: 3, 4), attempting format-3-style parsing", formatVersion);

    if (depth == 0)
    {
        out.modelType = type;
        out.formatVersion = formatVersion;
        out.bboxMin = {bboxMin[0], bboxMin[1], bboxMin[2]};
        out.bboxMax = {bboxMax[0], bboxMax[1], bboxMax[2]};
    }

    const ogf_u32 verticesChunkId = VerticesChunkId(formatVersion);
    const ogf_u32 indicesChunkId = IndicesChunkId(formatVersion);

    switch (ClassifyOgfType(formatVersion, type))
    {
    case EOgfKind::Normal:
    {
        SOgfMeshDef mesh;
        LoadTexture(r, mesh);
        if (!LoadStaticGeometry(r, mesh, verticesChunkId, indicesChunkId))
            return false;
        out.meshes.push_back(std::move(mesh));
        break;
    }

    case EOgfKind::SkeletonGeomdefST:
    case EOgfKind::SkeletonGeomdefPM: // TODO: progressive LOD (OGF_SWIDATA) not yet used, full-res geometry is loaded instead
    {
        SOgfMeshDef mesh;
        LoadTexture(r, mesh);
        if (!LoadSkinnedGeometry(r, mesh, formatVersion, verticesChunkId, indicesChunkId))
            return false;
        out.meshes.push_back(std::move(mesh));
        break;
    }

    case EOgfKind::Hierarchy:
    case EOgfKind::SkeletonAnim:
    case EOgfKind::SkeletonRigid:
    {
        //-- MT_SKELETON_RIGID is itself a container (CKinematics), same as
        //-- MT_SKELETON_ANIM (CKinematicsAnimated) - both have OGF_S_BONE_NAMES
        //-- + OGF_CHILDREN, the difference is only that RIGID has no motions.
        //-- Only the individual children (GEOMDEF_ST/PM) are leaf meshes.
        const EOgfKind kind = ClassifyOgfType(formatVersion, type);
        if (kind == EOgfKind::SkeletonAnim || kind == EOgfKind::SkeletonRigid)
            LoadSkeleton(r, out, formatVersion);

        //-- motion references are only meaningful (and only present) on the
        //-- root visual - a nested child would never carry its own OGF_S_MOTION_REFS
        if (depth == 0 && (kind == EOgfKind::SkeletonAnim || kind == EOgfKind::SkeletonRigid))
        {
            LoadMotionRefs(r, formatVersion, out);

            //-- the "release" engine (OGSR/CS/CoP) always splits motions out
            //-- into separate .omf files referenced above, but some SDK/preview
            //-- builds could in principle embed OGF_S_SMPARAMS/OGF_S_MOTIONS
            //-- directly in the .ogf's own top-level stream. Try that too, in
            //-- addition to (not instead of) the referenced .omf files - costs
            //-- nothing when absent (LoadSMParamsAndMotions just returns false,
            //-- silently since logIfMissing=false).
            const size_t before = out.motions.size();
            if (LoadSMParamsAndMotions(r, out.motions, out.sourcePath, /*logIfMissing=*/false, /*formatVersionHint=*/formatVersion))
                LogMsg("-COgfLoader::LoadVisual: [%s] also has %zu motion(s) embedded directly in the .ogf", out.sourcePath.c_str(), out.motions.size() - before);
        }

        const ogf_u32 childrenChunkId = ChildrenChunkId(formatVersion);
        COgfChunkedReader childrenContainer;
        if (r.open_chunk(childrenChunkId, childrenContainer))
        {
            int idx = 0;
            COgfChunkedReader child;
            while (childrenContainer.open_chunk(static_cast<ogf_u32>(idx), child))
            {
                if (!LoadVisual(child, out, depth + 1))
                    LogMsg("~COgfLoader: failed to load child #%d", idx);
                ++idx;
            }
            if (idx == 0)
                LogMsg("~COgfLoader: children chunk present but empty");
        }
        else
        {
            LogMsg("!COgfLoader: hierarchy visual has no children chunk [%u]", childrenChunkId);
            return depth > 0 ? true : false; // tolerate an empty nested node, fail on an empty root
        }
        break;
    }

    default:
        LogMsg("~COgfLoader: visual type [%u] (format v%u) is not supported yet, skipping", type, formatVersion);
        break;
    }

    return true;
}
