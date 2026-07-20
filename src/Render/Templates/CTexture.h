//----------------------------------------------------------------------------
//-- CTexture.cpp
//-- VlaGan: class implementation for mesh textures
//----------------------------------------------------------------------------
#pragma once

#include <string>
#include <d3d11.h>
#include <DirectXTex/DirectXTex.h>

class CTexture
{
public:
    CTexture();
    ~CTexture();

    CTexture(const CTexture&) = delete;
    CTexture& operator=(const CTexture&) = delete;

    CTexture(CTexture&& other) noexcept;
    CTexture& operator=(CTexture&& other) noexcept;

    bool LoadFromFile(ID3D11Device* device, const std::string& filename);

    void Release();

    ID3D11ShaderResourceView* GetSRV() const { return m_pSRV; }
    bool IsLoaded() const { return m_pSRV != nullptr; }
    std::string GetFileName() { return m_FileName; }
    bool IsTextureTransparent() { return m_bTransparent; }
    //void Bind(ID3D11DeviceContext* context)

    UINT GetWidth() { return m_Width; }
    UINT GetHeight() { return m_Height; }

private:
    bool stbiHasTransparency(const unsigned char* data, int width, int height, int channels);
    bool Load(ID3D11Device* device,  const std::string& filename);

    bool DDSHasTransparency(DirectX::ScratchImage& image);
    bool LoadDDS(ID3D11Device* device, const std::string& filename);

private:
    std::string m_FileName{};
    ID3D11Resource* m_pResource{};
    ID3D11ShaderResourceView* m_pSRV{};

    UINT m_Width{};
    UINT m_Height{};
    bool m_bTransparent{};

    // DDS-specific metadata
    int m_Depth{ 1 };
    int m_ArraySize{ 1 };
    int m_MipLevels{ 1 };
    DXGI_FORMAT m_Format{ DXGI_FORMAT_UNKNOWN };
    bool m_bIsCubeMap{};
    bool m_bIsVolumeMap{};
};
