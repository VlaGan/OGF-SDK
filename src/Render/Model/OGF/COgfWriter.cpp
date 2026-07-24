//----------------------------------------------------------------------------
//-- COgfWriter.cpp
//--
//-- KNOWN SIMPLIFICATIONS (round-trip is "very close", not byte-exact):
//--  - IK-data is always written as the newest per-format_version sub-variant
//--    (OGF_S_IKDATA for v4, OGF3_S_IKDATA_2 for v3 LegacySDK - byte-identical
//--    body, only the chunk id differs)
//--  - CSCoP mode always writes 4-link skin vertices, SoC mode always writes
//--    2-link, regardless of how many the source actually used - see
//--    WriteSkinnedGeometry for the SoC weight-collapse
//--  - motion keyframes are RE-quantized from the floats COgfLoader already
//--    dequantized, so re-exported .omf motions are extremely close but not
//--    byte-exact
//--  - SMPARAMS bone "slot" numbers are always written sequentially in
//--    partition order rather than reusing the source file's numbering
//--  - SaveMotions() can't split motions back across multiple referenced
//--    .omf files if the source had more than one ref (see its own comment)
//--  - a 4th, rarer motion-key mode exists in real files (uncompressed float
//--    keys, flTKeyFFT_Bit in OGF-tool's terms) that COgfLoader doesn't parse
//--    at all yet, so it can't round-trip through us either way
//----------------------------------------------------------------------------
#include "COgfWriter.h"
#include "COgfChunkWriter.h"

#include <fstream>
#include <algorithm>
#include <cmath>

#include "../../../_defines.h" // LogMsg

namespace
{
    //-- chunk-id pairs, mirrors the anonymous-namespace helpers in
    //-- COgfLoader.cpp (same names, same logic, just the write-side twin).
    //-- These vary ONLY with format_version (3 vs 4) - SoC and CS/CoP share
    //-- every one of these, since both are format_version 4.
    ogf_u32 VerticesChunkId(ogf_u8 fv) { return fv == OGF_FORMAT_VERSION ? (ogf_u32)OGF_VERTICES : (ogf_u32)OGF3_VERTICES; }
    ogf_u32 IndicesChunkId(ogf_u8 fv) { return fv == OGF_FORMAT_VERSION ? (ogf_u32)OGF_INDICES : (ogf_u32)OGF3_INDICES; }
    ogf_u32 ChildrenChunkId(ogf_u8 fv) { return fv == OGF_FORMAT_VERSION ? (ogf_u32)OGF_CHILDREN : (ogf_u32)OGF3_CHILDREN; }
    ogf_u32 IkDataChunkId(ogf_u8 fv) { return fv == OGF_FORMAT_VERSION ? (ogf_u32)OGF_S_IKDATA : (ogf_u32)OGF3_S_IKDATA_2; }
    ogf_u32 DescChunkId(ogf_u8 fv) { return fv == OGF_FORMAT_VERSION ? (ogf_u32)OGF_S_DESC : (ogf_u32)OGF3_S_DESC; }
    ogf_u32 UserDataChunkId(ogf_u8 fv) { return fv == OGF_FORMAT_VERSION ? (ogf_u32)OGF_S_USERDATA : (ogf_u32)OGF3_S_USERDATA; }
    ogf_u32 LodsChunkId(ogf_u8 fv) { return fv == OGF_FORMAT_VERSION ? (ogf_u32)OGF_S_LODS : (ogf_u32)OGF3_LODS; }
    ogf_u32 SmParamsChunkId(ogf_u8 fv) { return fv == OGF_FORMAT_VERSION ? (ogf_u32)OGF_S_SMPARAMS : (ogf_u32)OGF3_S_SMPARAMS_NEW; }
    ogf_u32 MotionsChunkId(ogf_u8 fv) { return fv == OGF_FORMAT_VERSION ? (ogf_u32)OGF_S_MOTIONS : (ogf_u32)OGF3_S_MOTIONS_NEW; }

    //-- single-string motion-refs chunk id - used whenever "SoC-style" refs
    //-- are requested, at EITHER format_version (matches OGF-tool's own
    //-- `Header.format_version == 4 ? OGF4_S_MOTION_REFS : OGF3_S_MOTION_REFS`)
    ogf_u32 SocMotionRefsChunkId(ogf_u8 fv) { return fv == OGF_FORMAT_VERSION ? (ogf_u32)OGF_S_MOTION_REFS : (ogf_u32)OGF3_S_MOTION_REFS; }

    constexpr size_t kMaxBonesSoC = 64; // vanilla SoC's known-safe skeleton size (raised in OGSR)
}

//----------------------------------------------------------------------------
ogf_u8 COgfWriter::TypeForKind(ogf_u8 formatVersion, EOgfWriteKind kind)
{
    if (formatVersion == OGF_FORMAT_VERSION)
    {
        switch (kind)
        {
        case EOgfWriteKind::Normal:            return MT_NORMAL;
        case EOgfWriteKind::Hierarchy:         return MT_HIERRARHY;
        case EOgfWriteKind::SkeletonAnim:      return MT_SKELETON_ANIM;
        case EOgfWriteKind::SkeletonGeomdefST: return MT_SKELETON_GEOMDEF_ST;
        case EOgfWriteKind::SkeletonRigid:     return MT_SKELETON_RIGID;
        }
    }
    //-- format_version 3 (LegacySDK): numerically DIFFERENT type values for
    //-- the same visual kind - see EOgfModelTypeV3 in ogf_format.h
    switch (kind)
    {
    case EOgfWriteKind::Normal:            return MT3_NORMAL;
    case EOgfWriteKind::Hierarchy:         return MT3_HIERRARHY;
    case EOgfWriteKind::SkeletonAnim:      return MT3_SKELETON_ANIM;
    case EOgfWriteKind::SkeletonGeomdefST: return MT3_SKELETON_GEOMDEF_ST;
    case EOgfWriteKind::SkeletonRigid:     return MT3_SKELETON_RIGID;
    }
    return MT3_NORMAL;
}

//----------------------------------------------------------------------------
void COgfWriter::WriteHeader(COgfChunkWriter& w, ogf_u8 formatVersion, ogf_u8 type, const DirectX::XMFLOAT3& bmin, const DirectX::XMFLOAT3& bmax)
{
    w.open_chunk(OGF_HEADER);
    w.w_u8(formatVersion);
    w.w_u8(type);
    w.w_u16(0); // shader_id - unused, matches COgfLoader ignoring it on read

    //-- format_version 3's header is only 4 bytes - no bbox/bsphere at all
    if (formatVersion == OGF_FORMAT_VERSION)
    {
        float mn[3] = { bmin.x, bmin.y, bmin.z };
        float mx[3] = { bmax.x, bmax.y, bmax.z };
        w.w_fvector3(mn);
        w.w_fvector3(mx);

        const DirectX::XMFLOAT3 center((bmin.x + bmax.x) * 0.5f, (bmin.y + bmax.y) * 0.5f, (bmin.z + bmax.z) * 0.5f);
        const float dx = bmax.x - center.x, dy = bmax.y - center.y, dz = bmax.z - center.z;
        const float radius = std::sqrt(dx * dx + dy * dy + dz * dz);
        float c[3] = { center.x, center.y, center.z };
        w.w_fvector3(c);
        w.w_float(radius);
    }

    w.close_chunk();
}

//----------------------------------------------------------------------------
void COgfWriter::WriteObb(COgfChunkWriter& w, const SOgfObb& obb)
{
    float v[3];
    v[0] = obb.rotationI.x; v[1] = obb.rotationI.y; v[2] = obb.rotationI.z; w.w_fvector3(v);
    v[0] = obb.rotationJ.x; v[1] = obb.rotationJ.y; v[2] = obb.rotationJ.z; w.w_fvector3(v);
    v[0] = obb.rotationK.x; v[1] = obb.rotationK.y; v[2] = obb.rotationK.z; w.w_fvector3(v);
    v[0] = obb.translate.x; v[1] = obb.translate.y; v[2] = obb.translate.z; w.w_fvector3(v);
    v[0] = obb.halfsize.x;  v[1] = obb.halfsize.y;  v[2] = obb.halfsize.z;  w.w_fvector3(v);
}

void COgfWriter::WriteBoneShape(COgfChunkWriter& w, const SOgfBoneShape& shape)
{
    w.w_u16(static_cast<ogf_u16>(shape.type));
    w.w_u16(shape.flags);
    WriteObb(w, shape.box);

    float v[3] = { shape.sphere.center.x, shape.sphere.center.y, shape.sphere.center.z };
    w.w_fvector3(v);
    w.w_float(shape.sphere.radius);

    v[0] = shape.cylinder.center.x; v[1] = shape.cylinder.center.y; v[2] = shape.cylinder.center.z;
    w.w_fvector3(v);
    v[0] = shape.cylinder.direction.x; v[1] = shape.cylinder.direction.y; v[2] = shape.cylinder.direction.z;
    w.w_fvector3(v);
    w.w_float(shape.cylinder.height);
    w.w_float(shape.cylinder.radius);
}

//----------------------------------------------------------------------------
void COgfWriter::WriteTexture(COgfChunkWriter& w, const SOgfMeshDef& mesh)
{
    w.open_chunk(OGF_TEXTURE); // chunk id 2 is shared between v3 and v4
    w.w_stringZ(mesh.textureName);
    w.w_stringZ(mesh.shaderName);
    w.close_chunk();
}

//----------------------------------------------------------------------------
void COgfWriter::WriteStaticGeometry(COgfChunkWriter& w, const SOgfMeshDef& mesh, ogf_u8 formatVersion)
{
    w.open_chunk(VerticesChunkId(formatVersion));

    const ogf_u32 fvf = OGF_D3DFVF_XYZ | OGF_D3DFVF_NORMAL | (1u << OGF_D3DFVF_TEXCOUNT_SHIFT);
    w.w_u32(fvf);
    w.w_u32(static_cast<ogf_u32>(mesh.vertices.size()));

    for (const Vertex& v : mesh.vertices)
    {
        float p[3] = { v.position.x, v.position.y, v.position.z };
        w.w_fvector3(p);
        float n[3] = { v.normal.x, v.normal.y, v.normal.z };
        w.w_fvector3(n);
        float uv[2] = { v.texcoord.x, v.texcoord.y };
        w.w_fvector2(uv);
    }

    w.close_chunk();
}

//----------------------------------------------------------------------------
//-- CS/CoP (socLinks=false): 4-link (ogf_vert_4w), vertType field written as
//-- a plain integer "4".
//-- SoC (socLinks=true): 2-link (ogf_vert_2w), vertType field written as the
//-- MAGIC-MULTIPLE form (2 * 0x12071980, i.e. OGF_VERTEXFORMAT_FVF_2L) - this
//-- is what real SoC assets actually use (confirmed: OGF-tool's own converter
//-- multiplies BY 0x12071980 when converting a model TO SoC, and divides when
//-- converting FROM SoC). Per-vertex weights are collapsed to the 2 heaviest
//-- influences and renormalized - lossy only for genuinely 3-4-bone verts.
//----------------------------------------------------------------------------
void COgfWriter::WriteSkinnedGeometry(COgfChunkWriter& w, const SOgfMeshDef& mesh, ogf_u8 formatVersion, bool socLinks)
{
    w.open_chunk(VerticesChunkId(formatVersion));

    const bool haveTB = mesh.tangents.size() == mesh.vertices.size() && mesh.binormals.size() == mesh.vertices.size();

    w.w_u32(socLinks ? static_cast<ogf_u32>(OGF_VERTEXFORMAT_FVF_2L) : 4u);
    w.w_u32(static_cast<ogf_u32>(mesh.vertices.size()));

    for (size_t i = 0; i < mesh.vertices.size(); ++i)
    {
        const Vertex& v = mesh.vertices[i];

        if (socLinks)
        {
            // pick the 2 heaviest influences out of the up-to-4 the source carries
            int idx[4] = { 0, 1, 2, 3 };
            std::sort(idx, idx + 4, [&](int a, int b) { return v.weights[a] > v.weights[b]; });

            const int bone0 = v.boneIDs[idx[0]] < 0 ? 0 : v.boneIDs[idx[0]];
            const int bone1 = v.boneIDs[idx[1]] < 0 ? 0 : v.boneIDs[idx[1]];
            const float w0 = v.weights[idx[0]], w1 = v.weights[idx[1]];
            const float sum = w0 + w1;
            const float wNorm = sum > 0.f ? (w1 / sum) : 0.f; // ogf_vert_2w: bone1 carries `w`, bone0 carries 1-w

            ogf_vert_2w s{};
            s.bone0 = static_cast<ogf_u16>(bone0);
            s.bone1 = static_cast<ogf_u16>(bone1);
            s.P[0] = v.position.x; s.P[1] = v.position.y; s.P[2] = v.position.z;
            s.N[0] = v.normal.x;   s.N[1] = v.normal.y;   s.N[2] = v.normal.z;
            if (haveTB)
            {
                s.T[0] = mesh.tangents[i].x;  s.T[1] = mesh.tangents[i].y;  s.T[2] = mesh.tangents[i].z;
                s.B[0] = mesh.binormals[i].x; s.B[1] = mesh.binormals[i].y; s.B[2] = mesh.binormals[i].z;
            }
            s.w = wNorm;
            s.u = v.texcoord.x; s.v = v.texcoord.y;
            w.w(&s, sizeof(s));
        }
        else
        {
            ogf_vert_4w s{};
            for (int b = 0; b < 4; ++b)
                s.bone[b] = static_cast<ogf_u16>(v.boneIDs[b] < 0 ? 0 : v.boneIDs[b]);

            s.P[0] = v.position.x; s.P[1] = v.position.y; s.P[2] = v.position.z;
            s.N[0] = v.normal.x;   s.N[1] = v.normal.y;   s.N[2] = v.normal.z;
            if (haveTB)
            {
                s.T[0] = mesh.tangents[i].x;  s.T[1] = mesh.tangents[i].y;  s.T[2] = mesh.tangents[i].z;
                s.B[0] = mesh.binormals[i].x; s.B[1] = mesh.binormals[i].y; s.B[2] = mesh.binormals[i].z;
            }
            s.w[0] = v.weights[0];
            s.w[1] = v.weights[1];
            s.w[2] = v.weights[2];
            s.u = v.texcoord.x; s.v = v.texcoord.y;
            w.w(&s, sizeof(s));
        }
    }

    w.close_chunk();
}

//----------------------------------------------------------------------------
void COgfWriter::WriteIndices(COgfChunkWriter& w, const SOgfMeshDef& mesh, ogf_u8 formatVersion)
{
    w.open_chunk(IndicesChunkId(formatVersion));
    w.w_u32(static_cast<ogf_u32>(mesh.indices.size()));
    for (uint32_t idx : mesh.indices)
    {
        if (idx > 0xFFFFu)
            LogMsg(eLogLevel::WARNING, "~COgfWriter::WriteIndices: index [%u] overflows 16-bit .ogf index format", idx);
        w.w_u16(static_cast<uint16_t>(idx));
    }
    w.close_chunk();
}

//----------------------------------------------------------------------------
void COgfWriter::WriteSkeleton(COgfChunkWriter& w, const SOgfModel& model, ogf_u8 formatVersion)
{
    w.open_chunk(OGF_S_BONE_NAMES); // chunk id 13 is shared between v3 and v4
    w.w_u32(static_cast<ogf_u32>(model.bones.size()));
    for (const SOgfBoneDef& bone : model.bones)
    {
        w.w_stringZ(bone.name);
        const std::string parentName = (bone.parentIndex >= 0 && bone.parentIndex < (int)model.bones.size())
            ? model.bones[bone.parentIndex].name : std::string();
        w.w_stringZ(parentName);
        WriteObb(w, bone.obb);
    }
    w.close_chunk();

    //-- OGF_S_IKDATA (v4) and OGF3_S_IKDATA_2 (v3's newest sub-version) share
    //-- the exact same body layout - only the chunk id differs
    w.open_chunk(IkDataChunkId(formatVersion));
    for (const SOgfBoneDef& bone : model.bones)
    {
        w.w_u32(bone.ikData.frictionValid ? 1u : 0u);
        w.w_stringZ(bone.material);
        WriteBoneShape(w, bone.shape);

        w.w_u32(bone.ikData.jointType);
        for (int a = 0; a < 3; ++a)
        {
            const SOgfJointLimit& lim = bone.ikData.limits[a];
            float lv[2] = { lim.limit.x, lim.limit.y };
            w.w_fvector2(lv);
            w.w_float(lim.springFactor);
            w.w_float(lim.dampingFactor);
        }
        w.w_float(bone.ikData.springFactor);
        w.w_float(bone.ikData.dampingFactor);
        w.w_u32(bone.ikData.ikFlags);
        w.w_float(bone.ikData.breakForce);
        w.w_float(bone.ikData.breakTorque);
        if (bone.ikData.frictionValid)
            w.w_float(bone.ikData.friction);

        float rot[3] = { bone.rotation.x, bone.rotation.y, bone.rotation.z };
        w.w_fvector3(rot);
        float pos[3] = { bone.position.x, bone.position.y, bone.position.z };
        w.w_fvector3(pos);
        w.w_float(bone.mass);
        float com[3] = { bone.centerOfMass.x, bone.centerOfMass.y, bone.centerOfMass.z };
        w.w_fvector3(com);
    }
    w.close_chunk();
}

//----------------------------------------------------------------------------
void COgfWriter::WriteDescription(COgfChunkWriter& w, const SOgfModel& model, ogf_u8 formatVersion)
{
    if (!model.description.present)
        return;

    const SOgfDescription& d = model.description;
    auto writeTimer = [&](int64_t t) {
        if (d.fourByteTimers)
            w.w_u32(static_cast<uint32_t>(t));
        else
        {
            w.w_u32(static_cast<uint32_t>(t & 0xFFFFFFFFll));
            w.w_u32(static_cast<uint32_t>((t >> 32) & 0xFFFFFFFFll));
        }
        };

    w.open_chunk(DescChunkId(formatVersion));
    w.w_stringZ(d.sourceFile);
    w.w_stringZ(d.exportTool);
    writeTimer(d.exportTime);
    w.w_stringZ(d.ownerName);
    writeTimer(d.creationTime);
    w.w_stringZ(d.lastModifiedByTool);
    writeTimer(d.modifiedTime);
    w.close_chunk();
}

//----------------------------------------------------------------------------
//-- reverse of COgfLoader::LoadSMParamsAndMotions.
//-- socMotions=true forces 8-bit translation keys (never 16-bit) - vanilla
//-- SoC's own tooling doesn't even requantize these, it just deletes
//-- incompatible motions outright ("Delete motions?" prompt); we requantize
//-- instead, which is strictly better.
//----------------------------------------------------------------------------
void COgfWriter::WriteSMParamsAndMotions(COgfChunkWriter& w, const SOgfModel& model, ogf_u8 formatVersion, bool socMotions)
{
    const bool use16BitTranslation = !socMotions;

    w.open_chunk(SmParamsChunkId(formatVersion));
    w.w_u16(OGF_SMPARAMS_VERSION); // always write the newest sub-version (marks supported)
    w.w_u16(static_cast<ogf_u16>(model.partitions.size()));

    ogf_u32 slot = 0;
    for (const SOgfPartition& part : model.partitions)
    {
        w.w_stringZ(part.name);
        w.w_u16(static_cast<ogf_u16>(part.boneNames.size()));
        for (const std::string& boneName : part.boneNames)
        {
            w.w_stringZ(boneName);
            w.w_u32(slot++);
        }
    }

    w.w_u16(static_cast<ogf_u16>(model.motionDefs.size()));
    for (const SOgfMotionDef& def : model.motionDefs)
    {
        w.w_stringZ(def.name);
        w.w_u32(static_cast<ogf_u32>(def.flags));
        w.w_u16(def.boneOrPart);
        w.w_u16(def.motionIndex);
        w.w_float(def.speed);
        w.w_float(def.power);
        w.w_float(def.accrue);
        w.w_float(def.falloff);

        w.w_u32(static_cast<ogf_u32>(def.marks.size()));
        for (const SOgfMotionMark& mark : def.marks)
        {
            w.w_stringA(mark.name);
            w.w_u32(static_cast<ogf_u32>(mark.intervals.size()));
            for (const DirectX::XMFLOAT2& iv : mark.intervals)
            {
                w.w_float(iv.x);
                w.w_float(iv.y);
            }
        }
    }
    w.close_chunk(); // SMPARAMS

    w.open_chunk(MotionsChunkId(formatVersion));

    w.open_chunk(0);
    w.w_u32(static_cast<ogf_u32>(model.motions.size()));
    w.close_chunk();

    for (size_t m = 0; m < model.motions.size(); ++m)
    {
        const CMotion& motion = model.motions[m];
        w.open_chunk(static_cast<uint32_t>(m + 1));

        w.w_stringZ(motion.m_sMotionName);
        const ogf_u32 frameCount = static_cast<ogf_u32>(motion.m_fDuration > 1.f ? motion.m_fDuration : 1.f);
        w.w_u32(frameCount);

        for (const BoneMotionData& bmd : motion.m_vBoneMotions)
        {
            const bool rotationConst = bmd.rotations.size() <= 1;
            const bool positionConst = bmd.positions.size() <= 1;

            uint8_t flags = 0;
            if (rotationConst) flags |= OGF_MOTION_FL_R_KEY_ABSENT;
            if (!positionConst)
            {
                flags |= OGF_MOTION_FL_T_KEY_PRESENT;
                if (use16BitTranslation)
                    flags |= OGF_MOTION_FL_T_KEY_16BIT;
            }
            w.w_u8(flags);

            //-- rotation - always 16-bit quantized quaternion in BOTH dialects
            auto quantRot = [](const DirectX::XMFLOAT4& q) {
                ogf_key_qr k{};
                k.x = static_cast<int16_t>(std::clamp(q.x * OGF_KEY_QUANT, -32767.f, 32767.f));
                k.y = static_cast<int16_t>(std::clamp(q.y * OGF_KEY_QUANT, -32767.f, 32767.f));
                k.z = static_cast<int16_t>(std::clamp(q.z * OGF_KEY_QUANT, -32767.f, 32767.f));
                k.w = static_cast<int16_t>(std::clamp(q.w * OGF_KEY_QUANT, -32767.f, 32767.f));
                return k;
                };

            if (rotationConst)
            {
                const DirectX::XMFLOAT4 q = bmd.rotations.empty() ? DirectX::XMFLOAT4(0, 0, 0, 1) : bmd.rotations[0].rotation;
                ogf_key_qr k = quantRot(q);
                w.w(&k, sizeof(k));
            }
            else
            {
                w.w_u32(0); // crc - unused on read, written as 0
                for (ogf_u32 f = 0; f < frameCount; ++f)
                {
                    const DirectX::XMFLOAT4& q = (f < bmd.rotations.size()) ? bmd.rotations[f].rotation : bmd.rotations.back().rotation;
                    ogf_key_qr k = quantRot(q);
                    w.w(&k, sizeof(k));
                }
            }

            //-- translation
            if (!positionConst)
            {
                w.w_u32(0); // crc

                DirectX::XMFLOAT3 minP = bmd.positions[0].position, maxP = minP;
                for (const auto& kp : bmd.positions)
                {
                    minP.x = std::min(minP.x, kp.position.x); maxP.x = std::max(maxP.x, kp.position.x);
                    minP.y = std::min(minP.y, kp.position.y); maxP.y = std::max(maxP.y, kp.position.y);
                    minP.z = std::min(minP.z, kp.position.z); maxP.z = std::max(maxP.z, kp.position.z);
                }

                const float quantMax = use16BitTranslation ? 32767.f : 127.f;
                const float centerX = (minP.x + maxP.x) * 0.5f, halfX = (maxP.x - minP.x) * 0.5f;
                const float centerY = (minP.y + maxP.y) * 0.5f, halfY = (maxP.y - minP.y) * 0.5f;
                const float centerZ = (minP.z + maxP.z) * 0.5f, halfZ = (maxP.z - minP.z) * 0.5f;

                const float sizeT[3] = { halfX / quantMax, halfY / quantMax, halfZ / quantMax };
                const float initT[3] = { centerX, centerY, centerZ };

                for (ogf_u32 f = 0; f < frameCount; ++f)
                {
                    const DirectX::XMFLOAT3& p = (f < bmd.positions.size()) ? bmd.positions[f].position : bmd.positions.back().position;
                    const float rx = sizeT[0] > 0.f ? std::clamp((p.x - initT[0]) / sizeT[0], -quantMax, quantMax) : 0.f;
                    const float ry = sizeT[1] > 0.f ? std::clamp((p.y - initT[1]) / sizeT[1], -quantMax, quantMax) : 0.f;
                    const float rz = sizeT[2] > 0.f ? std::clamp((p.z - initT[2]) / sizeT[2], -quantMax, quantMax) : 0.f;

                    if (use16BitTranslation)
                    {
                        ogf_key_qt16 k{};
                        k.x = static_cast<int16_t>(rx); k.y = static_cast<int16_t>(ry); k.z = static_cast<int16_t>(rz);
                        w.w(&k, sizeof(k));
                    }
                    else
                    {
                        ogf_key_qt8 k{};
                        k.x = static_cast<int8_t>(rx); k.y = static_cast<int8_t>(ry); k.z = static_cast<int8_t>(rz);
                        w.w(&k, sizeof(k));
                    }
                }

                w.w_fvector3(sizeT);
                w.w_fvector3(initT);
            }
            else
            {
                const DirectX::XMFLOAT3& p = bmd.positions.empty() ? DirectX::XMFLOAT3(0, 0, 0) : bmd.positions[0].position;
                float pf[3] = { p.x, p.y, p.z };
                w.w_fvector3(pf);
            }
        }

        w.close_chunk();
    }
    w.close_chunk(); // MOTIONS
}

//----------------------------------------------------------------------------
void COgfWriter::ComputeBounds(const SOgfMeshDef& mesh, DirectX::XMFLOAT3& outMin, DirectX::XMFLOAT3& outMax)
{
    if (mesh.vertices.empty())
    {
        outMin = outMax = DirectX::XMFLOAT3(0, 0, 0);
        return;
    }

    outMin = outMax = mesh.vertices[0].position;
    for (const Vertex& v : mesh.vertices)
    {
        outMin.x = std::min(outMin.x, v.position.x);
        outMin.y = std::min(outMin.y, v.position.y);
        outMin.z = std::min(outMin.z, v.position.z);
        outMax.x = std::max(outMax.x, v.position.x);
        outMax.y = std::max(outMax.y, v.position.y);
        outMax.z = std::max(outMax.z, v.position.z);
    }
}

//----------------------------------------------------------------------------
void COgfWriter::WriteMeshVisual(COgfChunkWriter& w, const SOgfMeshDef& mesh, bool skinned, ogf_u8 formatVersion, bool socLinks)
{
    DirectX::XMFLOAT3 bmin, bmax;
    ComputeBounds(mesh, bmin, bmax);

    const ogf_u8 type = TypeForKind(formatVersion, skinned ? EOgfWriteKind::SkeletonGeomdefST : EOgfWriteKind::Normal);

    WriteHeader(w, formatVersion, type, bmin, bmax);
    WriteTexture(w, mesh);
    if (skinned)
        WriteSkinnedGeometry(w, mesh, formatVersion, socLinks);
    else
        WriteStaticGeometry(w, mesh, formatVersion);
    WriteIndices(w, mesh, formatVersion);
}

//----------------------------------------------------------------------------
bool COgfWriter::Save(const std::string& path, const SOgfModel& model, EOgfModelFormat mode)
{
    if (model.meshes.empty())
    {
        LogMsg(eLogLevel::ERR, "!COgfWriter::Save: model has no meshes to write");
        return false;
    }

    const ogf_u8 formatVersion = (mode == EOgfModelFormat::eLegacySDK) ? 3 : OGF_FORMAT_VERSION;
    const bool socLinks = (mode == EOgfModelFormat::eSoC);

    //-- LegacySDK (v3) never had a REFS2-style chunk at all, so it always
    //-- uses the single-string form too - only real SoC vs CoP (both v4) is
    //-- an actual *choice* here
    const bool socMotionRefs = (mode == EOgfModelFormat::eSoC) || (mode == EOgfModelFormat::eLegacySDK);
    const bool socMotions = (mode == EOgfModelFormat::eSoC) || (mode == EOgfModelFormat::eLegacySDK);

    if (socLinks && model.bones.size() > kMaxBonesSoC)
        LogMsg(eLogLevel::WARNING, "~COgfWriter::Save: [%s] has %zu bones - vanilla SoC's hardware skinning is only known-safe up to %zu, expect a crash or corrupted skinning in-game",
            path.c_str(), model.bones.size(), kMaxBonesSoC);

    COgfChunkWriter w;

    const bool hasSkeleton = !model.bones.empty();
    EOgfWriteKind rootKind;
    if (hasSkeleton)
        rootKind = (model.motions.empty() && model.motionRefs.empty()) ? EOgfWriteKind::SkeletonRigid : EOgfWriteKind::SkeletonAnim;
    else if (model.meshes.size() > 1)
        rootKind = EOgfWriteKind::Hierarchy;
    else
        rootKind = EOgfWriteKind::Normal;

    const ogf_u8 rootType = TypeForKind(formatVersion, rootKind);
    WriteHeader(w, formatVersion, rootType, model.bboxMin, model.bboxMax);

    if (!model.userData.empty())
    {
        w.open_chunk(UserDataChunkId(formatVersion));
        w.w_stringZ(model.userData);
        w.close_chunk();
    }

    WriteDescription(w, model, formatVersion);

    if (!model.lodPath.empty())
    {
        w.open_chunk(LodsChunkId(formatVersion));
        w.w_stringZ(model.lodPath);
        w.close_chunk();
    }

    if (rootKind == EOgfWriteKind::Normal)
    {
        WriteTexture(w, model.meshes[0]);
        WriteStaticGeometry(w, model.meshes[0], formatVersion);
        WriteIndices(w, model.meshes[0], formatVersion);
    }
    else
    {
        //-- CHILDREN must be written BEFORE bone/ik data - OGF-tool flags
        //-- files as "broken type 2" if OGF_S_BONE_NAMES appears earlier in
        //-- the file than OGF_CHILDREN
        w.open_chunk(ChildrenChunkId(formatVersion));
        for (size_t i = 0; i < model.meshes.size(); ++i)
        {
            COgfChunkWriter child;
            WriteMeshVisual(child, model.meshes[i], hasSkeleton, formatVersion, socLinks);
            w.open_chunk(static_cast<uint32_t>(i));
            w.w(child.buffer().data(), child.buffer().size());
            w.close_chunk();
        }
        w.close_chunk(); // CHILDREN

        if (hasSkeleton)
        {
            WriteSkeleton(w, model, formatVersion);

            if (!model.motionRefs.empty())
            {
                if (socMotionRefs)
                {
                    std::string joined;
                    for (size_t i = 0; i < model.motionRefs.size(); ++i)
                    {
                        if (i) joined += ',';
                        joined += model.motionRefs[i];
                    }
                    w.open_chunk(SocMotionRefsChunkId(formatVersion));
                    w.w_stringZ(joined);
                    w.close_chunk();
                }
                else
                {
                    w.open_chunk(OGF_S_MOTION_REFS2);
                    w.w_u32(static_cast<ogf_u32>(model.motionRefs.size()));
                    for (const std::string& ref : model.motionRefs)
                        w.w_stringZ(ref);
                    w.close_chunk();
                }
            }

            if (!model.motions.empty())
                WriteSMParamsAndMotions(w, model, formatVersion, socMotions);
        }
    }

    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file.is_open())
    {
        LogMsg(eLogLevel::ERR, "!COgfWriter::Save: cant open [%s] for writing", path.c_str());
        return false;
    }
    file.write(reinterpret_cast<const char*>(w.buffer().data()), w.buffer().size());

    static const char* kModeNames[] = { "Unknown", "CS/CoP", "SoC", "LegacySDK(v3)" };
    LogMsg("-COgfWriter::Save: [%s] (%s) -> %zu byte(s)", path.c_str(), kModeNames[static_cast<int>(mode)], w.buffer().size());
    return true;
}

//----------------------------------------------------------------------------
bool COgfWriter::SaveMotionsFile(const std::string& path, const SOgfModel& model, EOgfModelFormat mode)
{
    if (model.motions.empty() && model.partitions.empty())
        return false;

    const ogf_u8 formatVersion = (mode == EOgfModelFormat::eLegacySDK) ? 3 : OGF_FORMAT_VERSION;
    const bool socMotions = (mode == EOgfModelFormat::eSoC) || (mode == EOgfModelFormat::eLegacySDK);

    COgfChunkWriter w;
    WriteSMParamsAndMotions(w, model, formatVersion, socMotions);

    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file.is_open())
    {
        LogMsg(eLogLevel::ERR, "!COgfWriter::SaveMotionsFile: cant open [%s] for writing", path.c_str());
        return false;
    }
    file.write(reinterpret_cast<const char*>(w.buffer().data()), w.buffer().size());

    LogMsg("-COgfWriter::SaveMotionsFile: [%s] -> %zu motion(s)", path.c_str(), model.motions.size());
    return true;
}

//----------------------------------------------------------------------------
bool COgfWriter::SaveMotions(const std::string& ogfPath, const SOgfModel& model, EOgfModelFormat mode)
{
    if (model.motionRefs.empty() || model.motions.empty())
        return false;

    const size_t slash = ogfPath.find_last_of("/\\");
    const std::string dir = (slash == std::string::npos) ? std::string() : ogfPath.substr(0, slash + 1);

    if (model.motionRefs.size() > 1)
        LogMsg(eLogLevel::WARNING, "~COgfWriter::SaveMotions: [%s] has %zu motion refs, writing all %zu motion(s) into [%s] only",
            ogfPath.c_str(), model.motionRefs.size(), model.motions.size(), model.motionRefs[0].c_str());

    const std::string omfPath = dir + model.motionRefs[0] + ".omf";
    return SaveMotionsFile(omfPath, model, mode);
}
