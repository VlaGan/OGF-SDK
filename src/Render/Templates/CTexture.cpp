//----------------------------------------------------------------------------
//-- CTexture.cpp
//-- VlaGan: class implementation for mesh textures
//----------------------------------------------------------------------------
#include "CTexture.h"
#include "../../_defines.h"
#include <filesystem>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>

CTexture::CTexture(){}

CTexture::~CTexture() { Release(); }

void CTexture::Release()
{
    RELEASE(m_pSRV);
    RELEASE(m_pResource);
}

CTexture::CTexture(CTexture&& other) noexcept
{
    m_pSRV = std::move(other.m_pSRV);
    m_pResource = std::move(other.m_pResource);
    other.m_pSRV = nullptr;
    other.m_pResource = nullptr;

    m_FileName = other.m_FileName;
    m_Width = other.m_Width;
    m_Height = other.m_Height;
    m_bTransparent = other.m_bTransparent;
    m_Depth = other.m_Depth;
    m_ArraySize = other.m_ArraySize;
    m_MipLevels = other.m_MipLevels;
    m_Format = other.m_Format;
    m_bIsCubeMap = other.m_bIsCubeMap;
    m_bIsVolumeMap = other.m_bIsVolumeMap;
}

CTexture& CTexture::operator=(CTexture&& other) noexcept
{
    if (this != &other)
    {
        Release();

        m_pSRV = std::move(other.m_pSRV);
        m_pResource = std::move(other.m_pResource);
        other.m_pSRV = nullptr;
        other.m_pResource = nullptr;

        m_FileName = other.m_FileName;
        m_Width = other.m_Width;
        m_Height = other.m_Height;
        m_bTransparent = other.m_bTransparent;
        m_Depth = other.m_Depth;
        m_ArraySize = other.m_ArraySize;
        m_MipLevels = other.m_MipLevels;
        m_Format = other.m_Format;
        m_bIsCubeMap = other.m_bIsCubeMap;
        m_bIsVolumeMap = other.m_bIsVolumeMap;
    }
    return *this;
}


//----------------------------------------------------------------------------
//-- Loading texture
//----------------------------------------------------------------------------
bool CTexture::LoadFromFile(ID3D11Device* device, const std::string& filename)
{
    Release();

    m_FileName = filename;
    std::filesystem::path path(filename);

    //if (std::filesystem::exists(path)) {
    //    LogMsg(eLogLevel::ERR, "CTexture::LoadFromFile [%s] doesnt exists!", filename.c_str());
    //    return Load(device, filename);
    //}

    auto ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    //-- little hack, loading ogf with wrong gamedata folder or missing texture
    //-- also models with SRV==nullptr always take last pushed to ps texture on next frame
    if (ext == ".dds") {
        if (LoadDDS(device, filename))
            return true;
        else
            return Load(device, "$default$");
    }

    return Load(device, filename);
}

//----------------------------------------------------------------------------
//-- Loading texture using stb_image
//----------------------------------------------------------------------------
bool CTexture::stbiHasTransparency(const unsigned char* data, int width, int height, int channels)
{
    if (channels < 4) return false;

    for (int i = 0; i < width * height; ++i) {
        int alpha = data[i * 4 + 3];
        if (alpha < 255) return true; // alpha < threshold
    }
    return false;
}

bool CTexture::Load(ID3D11Device* device, const std::string& filename) {
    
    Release();
    //-- simple example texture loading (later should be CTexture class or CTexture container manager)
    //int x, y, channels_in_file;
    //stbi_info(filename.c_str(), &x, &y, &channels_in_file);
    //LogMsg("Texture [%s] has %d channels in file", filename.c_str(), channels_in_file);
    int w, h, channels;
    unsigned char* data = stbi_load(filename.c_str(), &w, &h, &channels, STBI_rgb_alpha);
    if (!data) {
        LogMsg(eLogLevel::ERR, "!CMesh::LoadTextureResource: cant load texture[%s] fall back to default[image.png]", filename.c_str());
        data = stbi_load("appdata/textures/image.png", &w, &h, &channels, STBI_rgb_alpha);
    }
    if (!data)
        return false;

    m_FileName = filename;
    m_Width = w;
    m_Height = h;

    m_bTransparent = stbiHasTransparency(data, w, h, channels);
    if (m_bTransparent)
        LogMsg("CTexture::Load -> Transparent texture found [%s]", filename.c_str());

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = w;
    desc.Height = h;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.SampleDesc.Count = 1;
    desc.MiscFlags = 0;
    desc.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA initTex = {};
    initTex.pSysMem = data;
    initTex.SysMemPitch = w * 4;

    ID3D11Texture2D* texture{};
    HRESULT hr = device->CreateTexture2D(&desc, &initTex, &texture);
    if (FAILED(hr)) {
        MessageBox(nullptr, L"CreateTexture2D failed", L"Error", MB_OK);
        stbi_image_free(data);
        return false;
    }

    hr = device->CreateShaderResourceView(texture, nullptr, &m_pSRV);
    if (FAILED(hr)) {
        MessageBox(nullptr, L"CreateShaderResourceView failed", L"Error", MB_OK);
        texture->Release();
        stbi_image_free(data);
        return false;
    }
    if (texture)
        texture->Release();
    stbi_image_free(data);

    return true;
}


//----------------------------------------------------------------------------
//-- Loading texture using DirectXTex
//----------------------------------------------------------------------------
bool CTexture::DDSHasTransparency(DirectX::ScratchImage& image)
{
    auto* images = image.GetImages();
    size_t imageCount = image.GetImageCount();
    if (imageCount == 0) return false;

    const DirectX::Image& img = images[0];
    if (!DirectX::HasAlpha(img.format))
        return false;

    const uint8_t* pixelData = static_cast<const uint8_t*>(img.pixels);
    size_t pixelCount = img.width * img.height;

    size_t bytesPerPixel = DirectX::BitsPerPixel(img.format) / 8;
    size_t alphaOffset = 3;

    for (size_t i = 0; i < pixelCount; ++i)
    {
        size_t pixelOffset = i * bytesPerPixel;
        uint8_t alpha = pixelData[pixelOffset + alphaOffset];

        if (alpha < 255)
            return true;
    }

    return false;
}

bool CTexture::LoadDDS(ID3D11Device* device, const std::string& filename)
{
    Release();

    std::filesystem::path path(filename);

    DirectX::ScratchImage image;
    DirectX::TexMetadata metadata;

    HRESULT hr = DirectX::LoadFromDDSFile(path.c_str(), DirectX::DDS_FLAGS_NONE, &metadata, image);

    if (FAILED(hr))
    {
        LogMsg(eLogLevel::ERR, "CTexture::LoadDDS: failed to load [%s]", filename.c_str());
        return false;
    }

    //------------------------------------------
    // Save metadata
    //------------------------------------------
    m_FileName = filename;
    m_Width = static_cast<UINT>(metadata.width);
    m_Height = static_cast<UINT>(metadata.height);

    m_Depth = static_cast<int>(metadata.depth);
    m_ArraySize = static_cast<int>(metadata.arraySize);
    m_MipLevels = static_cast<int>(metadata.mipLevels);

    m_Format = metadata.format;

    m_bIsCubeMap = metadata.IsCubemap();
    m_bIsVolumeMap = metadata.IsVolumemap();

    //m_bTransparent = DDSHasTransparency(image);

    //------------------------------------------
    // Create SRV
    //------------------------------------------
    hr = DirectX::CreateShaderResourceView(device, image.GetImages(), 
        image.GetImageCount(), metadata, &m_pSRV);

    if (FAILED(hr))
    {
        LogMsg(eLogLevel::ERR, "CTexture::LoadDDS: CreateShaderResourceView failed [%s]", filename.c_str());
        return false;
    }

    m_pSRV->GetResource(&m_pResource);

    LogMsg("CTexture::LoadDDS [%s] %ux%u mip=%d format=%d", filename.c_str(), m_Width, m_Height, 
        m_MipLevels, m_Format);

    return true;
}

//----------------------------------------------------------------------------
//-- Set shader resource
//----------------------------------------------------------------------------
/*void CTexture::Bind(ID3D11DeviceContext* context) {

    if (!m_pSRV) {
        LogMsg(eLogLevel::ERR, "CTexture::Bind -> [%s] !m_pSRV", m_FileName.c_str());
        return;
    }
    context->PSSetShaderResources(0, 1, &m_pSRV);
}*/
//----------------------------------------------------------------------------
