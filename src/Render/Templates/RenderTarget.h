//----------------------------------------------------------------------------
//-- RenderTarget.h
//-- VlaGan: Basic rendertarget class .header
//----------------------------------------------------------------------------
#pragma once
#include <d3d11.h>

class CRenderTarget {
public:
    CRenderTarget();
    ~CRenderTarget();

    bool Create(ID3D11Device* device, UINT width, UINT height, DXGI_FORMAT tformat = DXGI_FORMAT_R8G8B8A8_UNORM);
    void Release();

    ID3D11Texture2D* GetTexture() { return m_Texture; }
    ID3D11RenderTargetView* GetRTV() { return m_RTV; }
    ID3D11ShaderResourceView* GetSRV() { return m_SRV; }

    ID3D11Texture2D* m_Texture{};
    ID3D11RenderTargetView* m_RTV{};
    ID3D11ShaderResourceView* m_SRV{};

    UINT m_Width{};
    UINT m_Height{};
    DXGI_FORMAT m_format{ DXGI_FORMAT_UNKNOWN };
};
