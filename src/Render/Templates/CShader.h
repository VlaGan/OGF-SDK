//----------------------------------------------------------------------------
//-- CShader.h
//-- VlaGan: Shader class implementation .header
//----------------------------------------------------------------------------
#pragma once

#include <d3d11.h>
#include <string>

class CShader {
public:
    CShader();
    ~CShader();

    CShader(const CShader&) = delete;
    CShader& operator=(const CShader&) = delete;

    CShader(CShader&& other) noexcept;
    CShader& operator=(CShader&& other) noexcept;


    bool LoadFromFile(ID3D11Device* device, const std::wstring& filePath,
        const std::string& vsEntry, const std::string& psEntry,
        const D3D11_INPUT_ELEMENT_DESC* layout, UINT layoutCount);

    bool LoadFromFile(ID3D11Device* device, const std::wstring& filePath,
        const std::string& vsEntry, const std::string& psEntry);

    void Bind(ID3D11DeviceContext* context, bool use_inlayot = true);

    ID3D11VertexShader* VS() const { return m_VS; }
    ID3D11PixelShader* PS() const { return m_PS; }
    ID3D11InputLayout* Layout() const { return m_Layout; }

    void Release();
    std::wstring m_Name{};

private:
    ID3D11VertexShader* m_VS{};
    ID3D11PixelShader* m_PS{};
    ID3D11InputLayout* m_Layout{};
};
