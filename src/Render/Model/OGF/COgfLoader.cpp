//----------------------------------------------------------------------------
//-- COgfLoader.cpp
//----------------------------------------------------------------------------
#include "COgfLoader.h"
#include "COgfChunkedReader.h"

#include <fstream>
#include <cstring>

extern void LogMsg(const char* fmt, ...);

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
