//--------------------------------------------------------------------------------------
//-- CShaderContainer - contains all shaders and manages their compilation and reloading
//--------------------------------------------------------------------------------------
#include "CShaderContainer.h"
#include <d3d11.h>
#include "Templates/CShader.h"

static std::wstring StringToWString(const std::string& str)
{
    if (str.empty())
        return std::wstring();

    int sizeNeeded = MultiByteToWideChar(
        CP_UTF8, 0,
        str.data(), (int)str.size(),
        nullptr, 0
    );

    if (sizeNeeded <= 0)
        return std::wstring();

    std::wstring result(sizeNeeded, 0);
    MultiByteToWideChar(
        CP_UTF8, 0,
        str.data(), (int)str.size(),
        result.data(), sizeNeeded
    );

    return result;
}

static std::string WStringToString(const std::wstring& wstr)
{
    if (wstr.empty())
        return std::string();

    int sizeNeeded = WideCharToMultiByte(
        CP_UTF8, 0,
        wstr.data(), (int)wstr.size(),
        nullptr, 0,
        nullptr, nullptr
    );

    if (sizeNeeded <= 0)
        return std::string();

    std::string result(sizeNeeded, 0);
    WideCharToMultiByte(
        CP_UTF8, 0,
        wstr.data(), (int)wstr.size(),
        result.data(), sizeNeeded,
        nullptr, nullptr
    );

    return result;
}


std::shared_ptr<CShader> CShaderContainer::GetOrLoad(
    ID3D11Device* device,
    const std::wstring& filePath,
    const std::string& vsEntry,
    const std::string& psEntry,
    D3D11_INPUT_ELEMENT_DESC* layout,
    UINT layoutCount)
{
    std::string key = WStringToString(filePath) + "|" + vsEntry + "|" + psEntry;

    auto it = m_Shaders.find(key);
    if (it != m_Shaders.end()) {
        return it->second;
    }

    auto shader = std::make_shared<CShader>();
    shader->LoadFromFile(device, filePath, vsEntry, psEntry, layout, layoutCount);
    m_Shaders[key] = shader;
    return shader;
}
