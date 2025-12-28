//----------------------------------------------------------------------------
//-- FullScreenQuad.cpp
//-- VlaGan: For rendering shader resource view on screen
//----------------------------------------------------------------------------
#include "FullScreenQuad.h"

void FullScreenQuad::Init(ID3D11Device* device) {
    FSVertex quadVertices[] = {
        { {-1.f,  1.f, 0.f}, {0.f, 0.f} },
        { { 1.f,  1.f, 0.f}, {1.f, 0.f} },
        { {-1.f, -1.f, 0.f}, {0.f, 1.f} },
        { { 1.f, -1.f, 0.f}, {1.f, 1.f} },
    };

    D3D11_BUFFER_DESC vbDesc = {};
    vbDesc.Usage = D3D11_USAGE_DEFAULT;
    vbDesc.ByteWidth = sizeof(quadVertices);
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = quadVertices;

    device->CreateBuffer(&vbDesc, &initData, &m_VertexBuffer);
}

void FullScreenQuad::Release() {
    RELEASE(m_VertexBuffer);
}

void FullScreenQuad::Render(ID3D11DeviceContext* context) {
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->Draw(6, 0); // <- without vertex/index buffer
}
