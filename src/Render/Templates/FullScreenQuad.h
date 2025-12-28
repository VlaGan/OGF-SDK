//----------------------------------------------------------------------------
//-- FullScreenQuad.h
//-- VlaGan: For rendering shader resource view on screen .header
//----------------------------------------------------------------------------
#pragma once
#include <d3d11.h>
#include <DirectXMath.h>
#include "../../_defines.h"


struct FSVertex {
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT2 texcoord;
};

class FullScreenQuad {
public:
    void Init(ID3D11Device* device);
    void Release();
    
    void Render(ID3D11DeviceContext* context);

private:
    ID3D11Buffer* m_VertexBuffer{};
};
