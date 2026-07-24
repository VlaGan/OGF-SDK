#pragma once
#include <string>
#include <DirectXMath.h>
#include "COgfModel.h"

class COgfChunkWriter;

class COgfWriter
{
public:
    //-- SoC and CS/CoP are BOTH format_version 4 (confirmed against a real
    //-- vanilla SoC .ogf, and against OGF-tool's Model.ChangeModelFormat()).
    //-- The real differences between them, straight from that function:
    //--   - max skin influences/vertex: 2 (SoC hw skinning limit) vs 4
    //--   - skin link-count field encoding: magic-multiple of 0x12071980
    //--     (SoC) vs plain integer (CoP) - see WriteSkinnedGeometry
    //--   - motion refs chunk: OGF_S_MOTION_REFS/single stringZ (SoC, id=19)
    //--     vs OGF_S_MOTION_REFS2/count+array (CoP, id=24)
    //--   - embedded motion translation keys: 8-bit (SoC) vs 16-bit (CoP)
    //--     (OGF-tool itself doesn't re-quantize these, it just offers to
    //--     delete incompatible motions outright - we requantize instead)
    //-- LegacySDK is the ACTUAL format_version 3 (pre-release SDK builds
    //-- 1469-1865) - a fully separate, older chunk-id namespace, unrelated
    //-- to the SoC/CoP split above.
    static bool Save(const std::string& path, const SOgfModel& model, EOgfModelFormat mode = EOgfModelFormat::eCSCoP);
    static bool SaveMotions(const std::string& ogfPath, const SOgfModel& model, EOgfModelFormat mode = EOgfModelFormat::eCSCoP);
    static bool SaveMotionsFile(const std::string& path, const SOgfModel& model, EOgfModelFormat mode = EOgfModelFormat::eCSCoP);

private:
    enum class EOgfWriteKind { Normal, Hierarchy, SkeletonAnim, SkeletonGeomdefST, SkeletonRigid };

    static ogf_u8 TypeForKind(ogf_u8 formatVersion, EOgfWriteKind kind);

    static void WriteHeader(COgfChunkWriter& w, ogf_u8 formatVersion, ogf_u8 type, const DirectX::XMFLOAT3& bmin, const DirectX::XMFLOAT3& bmax);
    static void WriteObb(COgfChunkWriter& w, const SOgfObb& obb);
    static void WriteBoneShape(COgfChunkWriter& w, const SOgfBoneShape& shape);
    static void WriteTexture(COgfChunkWriter& w, const SOgfMeshDef& mesh);
    static void WriteStaticGeometry(COgfChunkWriter& w, const SOgfMeshDef& mesh, ogf_u8 formatVersion);
    static void WriteSkinnedGeometry(COgfChunkWriter& w, const SOgfMeshDef& mesh, ogf_u8 formatVersion, bool socLinks);
    static void WriteIndices(COgfChunkWriter& w, const SOgfMeshDef& mesh, ogf_u8 formatVersion);
    static void WriteSkeleton(COgfChunkWriter& w, const SOgfModel& model, ogf_u8 formatVersion);
    static void WriteDescription(COgfChunkWriter& w, const SOgfModel& model, ogf_u8 formatVersion);
    static void WriteSMParamsAndMotions(COgfChunkWriter& w, const SOgfModel& model, ogf_u8 formatVersion, bool socMotions);
    static void WriteMeshVisual(COgfChunkWriter& w, const SOgfMeshDef& mesh, bool skinned, ogf_u8 formatVersion, bool socLinks);
    static void ComputeBounds(const SOgfMeshDef& mesh, DirectX::XMFLOAT3& outMin, DirectX::XMFLOAT3& outMax);
};
