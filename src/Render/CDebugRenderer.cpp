//----------------------------------------------------------------------------
//-- CDebugRenderer.cpp
//-- VlaGan: Debug render class implementation
//----------------------------------------------------------------------------
#include "CDebugRenderer.h"
#include "../_defines.h"

CDebugRenderer::~CDebugRenderer() {
    RELEASE(m_vertexBuffer);
}

void CDebugRenderer::Initialize(ID3D11Device* device) {

    D3D11_BUFFER_DESC vbDesc = {};
    vbDesc.ByteWidth = sizeof(DebugVertex) * m_maxVertices;
    vbDesc.Usage = D3D11_USAGE_DYNAMIC;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    device->CreateBuffer(&vbDesc, nullptr, &m_vertexBuffer);


    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };
    m_DebugShader = CShaderContainer::Get().GetOrLoad(device, L"appdata/shaders/debug.hlsl", "VSMain", "PSMain", layout, ARRAYSIZE(layout));

    m_constantBuffer.Create(device, sizeof(DirectX::XMMATRIX));

    m_pDevice = device;

	LogMsg(eLogLevel::DEBUG, "DebugRenderer initialized");
}

void CDebugRenderer::DrawLine(ID3D11DeviceContext* context, const DirectX::XMFLOAT3& start, const DirectX::XMFLOAT3& end, const DirectX::XMFLOAT4& color) {
    if (m_vertices[m_DebugPhase].size() + 2 > m_maxVertices) return;

    std::vector<DebugVertex>* v = &m_vertices[m_DebugPhase];

    m_vertices[m_DebugPhase].push_back({ start, color });
    m_vertices[m_DebugPhase].push_back({end, color});
}

void CDebugRenderer::DrawCube(ID3D11DeviceContext* context, const DirectX::XMFLOAT3& center, float size, const DirectX::XMFLOAT4& color) {
    float h = size * 0.5f;
    DirectX::XMFLOAT3 corners[8] = {
        { center.x - h, center.y - h, center.z - h },
        { center.x + h, center.y - h, center.z - h },
        { center.x - h, center.y + h, center.z - h },
        { center.x + h, center.y + h, center.z - h },
        { center.x - h, center.y - h, center.z + h },
        { center.x + h, center.y - h, center.z + h },
        { center.x - h, center.y + h, center.z + h },
        { center.x + h, center.y + h, center.z + h }
    };

    // bottom
    DrawLine(context, corners[0], corners[1], color);
    DrawLine(context, corners[1], corners[3], color);
    DrawLine(context, corners[3], corners[2], color);
    DrawLine(context, corners[2], corners[0], color);

    // top
    DrawLine(context, corners[4], corners[5], color);
    DrawLine(context, corners[5], corners[7], color);
    DrawLine(context, corners[7], corners[6], color);
    DrawLine(context, corners[6], corners[4], color);

    // from bot/top
    DrawLine(context, corners[0], corners[4], color);
    DrawLine(context, corners[1], corners[5], color);
    DrawLine(context, corners[2], corners[6], color);
    DrawLine(context, corners[3], corners[7], color);
}

void CDebugRenderer::DrawBoneAxis(ID3D11DeviceContext* context, const CBoneInstance& bone, float size) {
    DirectX::XMMATRIX boneWorld = bone.mGlobalTransform;
    DirectX::XMVECTOR bonePos = boneWorld.r[3];

    DirectX::XMVECTOR xAxis = XMVector3TransformNormal(DirectX::XMVectorSet(size, 0, 0, 0), boneWorld);
    DirectX::XMVECTOR yAxis = XMVector3TransformNormal(DirectX::XMVectorSet(0, size, 0, 0), boneWorld);
    DirectX::XMVECTOR zAxis = XMVector3TransformNormal(DirectX::XMVectorSet(0, 0, size, 0), boneWorld);

    DirectX::XMFLOAT3 pos, xEnd, yEnd, zEnd;
    DirectX::XMStoreFloat3(&pos, bonePos);
    DirectX::XMStoreFloat3(&xEnd, DirectX::XMVectorAdd(bonePos, xAxis));
    DirectX::XMStoreFloat3(&yEnd, DirectX::XMVectorAdd(bonePos, yAxis));
    DirectX::XMStoreFloat3(&zEnd, DirectX::XMVectorAdd(bonePos, zAxis));

    DrawLine(context, pos, xEnd, DirectX::XMFLOAT4(1, 0, 0, 1)); // X - r
    DrawLine(context, pos, yEnd, DirectX::XMFLOAT4(0, 1, 0, 1)); // Y - g
    DrawLine(context, pos, zEnd, DirectX::XMFLOAT4(0, 0, 1, 1)); // Z - b
}

//-- render all debug
void CDebugRenderer::Render(ID3D11DeviceContext* context, const DirectX::XMMATRIX& viewProj, bool depth) {
    if (m_vertices[m_DebugPhase].empty()) return;

    //-- vertex buffer (position/color)
    {
        RELEASE(m_vertexBuffer);

        D3D11_BUFFER_DESC bd = {};
        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.ByteWidth = UINT(sizeof(DebugVertex) * m_vertices[m_DebugPhase].size());
        bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        D3D11_SUBRESOURCE_DATA initData = {};
        initData.pSysMem = m_vertices[m_DebugPhase].data();
        HRESULT hr = m_pDevice->CreateBuffer(&bd, &initData, &m_vertexBuffer);

        UINT stride = sizeof(DebugVertex);
        UINT offset = 0;
        context->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
    }

    //-- constant buffer - viewproj matrix
    m_constantBuffer.Update(context, 0, nullptr, &viewProj, 0, 0);
    m_constantBuffer.VSSet(context, 0);

    m_DebugShader.get()->Bind(context);

    ID3D11DepthStencilState* oldDepthState;
    UINT oldStencilRef;
    if (!depth) {
        context->OMGetDepthStencilState(&oldDepthState, &oldStencilRef);

        D3D11_DEPTH_STENCIL_DESC depthDesc = {};
        depthDesc.DepthEnable = false;
        depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;

        ID3D11DepthStencilState* newDepthState;
        m_pDevice->CreateDepthStencilState(&depthDesc, &newDepthState);
        context->OMSetDepthStencilState(newDepthState, 0);
        newDepthState->Release();

        context->Draw((UINT)m_vertices[m_DebugPhase].size(), 0);

        context->OMSetDepthStencilState(oldDepthState, oldStencilRef);
        if (oldDepthState) oldDepthState->Release();
    } 
    else
        context->Draw((UINT)m_vertices[m_DebugPhase].size(), 0);

    m_vertices[m_DebugPhase].clear();
}
