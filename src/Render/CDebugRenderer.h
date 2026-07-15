//----------------------------------------------------------------------------
//-- CDebugRenderer.h
//-- VlaGan: Debug render class implementation
//----------------------------------------------------------------------------
#pragma once
#include <d3d11.h>
#include <DirectXMath.h>
#include "Model/CSkeleton.h"
#include "Templates/ConstantBuffer.h"
#include "templates/CShader.h"
#include "CShaderContainer.h"

struct DebugVertex {
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT4 color;
};

enum eDebugPhase : uint32_t {
    ePhaseBeforeScene = 0,
    ePhaseAfterScene,
    ePhaseTotal
};

class CDebugRenderer {
public:
    ~CDebugRenderer();
    void Initialize(ID3D11Device* device);
    void Shutdown();

    void DrawLine(ID3D11DeviceContext* context, const DirectX::XMFLOAT3& start, const DirectX::XMFLOAT3& end, const DirectX::XMFLOAT4& color);
    void DrawCube(ID3D11DeviceContext* context, const DirectX::XMFLOAT3& center, float size, const DirectX::XMFLOAT4& color);
    void DrawBoneAxis(ID3D11DeviceContext* context, const CBoneInstance& bone, float size = 0.5f);

    void Render(ID3D11DeviceContext* context, const DirectX::XMMATRIX& viewProj, bool depth = false);

    void SetRenderPhase(eDebugPhase phase) { m_DebugPhase = phase; }
    eDebugPhase m_DebugPhase{ ePhaseAfterScene };

private:
    ID3D11Buffer* m_vertexBuffer{};

    CConstantBuffer m_constantBuffer;
    std::shared_ptr<CShader> m_DebugShader;

    std::vector<DebugVertex> m_vertices[ePhaseTotal];
    //std::vector<DebugVertex> m_vertices;
    UINT m_maxVertices = 65536;
    ID3D11Device* m_pDevice{};
};
