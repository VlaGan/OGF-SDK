//----------------------------------------------------------------------------
//-- CShader.cpp
//-- VlaGan: Shader class implementation
//----------------------------------------------------------------------------
#include "CShader.h"
#include <d3dcompiler.h>
#include "../../_defines.h"

CShader::CShader() {}
CShader::~CShader() { Release(); }

CShader::CShader(CShader&& other) noexcept
{
    m_VS = other.m_VS;
    m_PS = other.m_PS;
    m_Layout = other.m_Layout;

    other.m_VS = nullptr;
    other.m_PS = nullptr;
    other.m_Layout = nullptr;
}

CShader& CShader::operator=(CShader&& other) noexcept
{
    if (this != &other)
    {
        Release();

        m_VS = other.m_VS;
        m_PS = other.m_PS;
        m_Layout = other.m_Layout;

        other.m_VS = nullptr;
        other.m_PS = nullptr;
        other.m_Layout = nullptr;
    }
    return *this;
}

bool CShader::LoadFromFile(ID3D11Device* device, const std::wstring& filePath,
    const std::string& vsEntry, const std::string& psEntry,
    const D3D11_INPUT_ELEMENT_DESC* layout, UINT layoutCount)
{
    ID3DBlob* vsBlob = nullptr;
    ID3DBlob* psBlob = nullptr;

    m_Name = filePath;

	LogMsg(WARNING, "CShader::LoadFromFile: Loading shader from file: %ls", filePath.c_str()); 

    HRESULT hr = D3DCompileFromFile(filePath.c_str(), nullptr, nullptr,
        vsEntry.c_str(), "vs_5_0", 0, 0, &vsBlob, nullptr);
    if (FAILED(hr)) {
        MessageBox(nullptr, L"CShader::LoadFromFile:[Vertex Shader(VS) loading error!]", L"Error", MB_OK);
        //return false;
    }

    hr = D3DCompileFromFile(filePath.c_str(), nullptr, nullptr,
        psEntry.c_str(), "ps_5_0", 0, 0, &psBlob, nullptr);
    if (FAILED(hr)) {
        MessageBox(nullptr, L"CShader::LoadFromFile:[Pixel shader(PS) loading error!]", L"Error", MB_OK);
        //return false;
    }

    hr = device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_VS);
    if (FAILED(hr)) return false;

    hr = device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_PS);
    if (FAILED(hr)) return false;

    hr = device->CreateInputLayout(layout, layoutCount, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &m_Layout);
    if (FAILED(hr)) return false;

    vsBlob->Release();
    psBlob->Release();

    return true;
}

bool CShader::LoadFromFile(ID3D11Device* device, const std::wstring& filePath,
    const std::string& vsEntry, const std::string& psEntry) {
    LogMsg(WARNING, "CShader::LoadFromFile: Loading shader from file: %ls", filePath.c_str());

    m_Name = filePath;

    ID3DBlob* vsBlob = nullptr;
    ID3DBlob* psBlob = nullptr;

    HRESULT hr = D3DCompileFromFile(filePath.c_str(), nullptr, nullptr,
        vsEntry.c_str(), "vs_5_0", 0, 0, &vsBlob, nullptr);
    if (FAILED(hr)) {
        MessageBox(nullptr, L"CShader::LoadFromFile:[Vertex Shader(VS) loading error!]", L"Error", MB_OK);
        //return false;
    }

    hr = D3DCompileFromFile(filePath.c_str(), nullptr, nullptr,
        psEntry.c_str(), "ps_5_0", 0, 0, &psBlob, nullptr);
    if (FAILED(hr)) {
        MessageBox(nullptr, L"CShader::LoadFromFile:[Pixel shader(PS) loading error!]", L"Error", MB_OK);
        //return false;
    }

    hr = device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_VS);
    if (FAILED(hr)) return false;

    hr = device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_PS);
    if (FAILED(hr)) return false;

    vsBlob->Release();
    psBlob->Release();

    return true;
}


void CShader::Bind(ID3D11DeviceContext* context, bool use_inlayot)
{
    if(use_inlayot)
        context->IASetInputLayout(m_Layout);
    context->VSSetShader(m_VS, nullptr, 0);
    context->PSSetShader(m_PS, nullptr, 0);
}

void CShader::Release()
{
    RELEASE(m_Layout);
    RELEASE(m_PS);
    RELEASE(m_VS);
}
