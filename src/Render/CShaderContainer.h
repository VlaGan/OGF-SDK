//--------------------------------------------------------------------------------------
//-- CShaderContainer - contains all shaders and manages their compilation and reloading
//--------------------------------------------------------------------------------------
#pragma once
#include <string>
#include <memory>
#include <unordered_map>
#include <wtypes.h>

class CShader;
struct ID3D11Device;
struct D3D11_INPUT_ELEMENT_DESC;

class CShaderContainer {
public:
    static CShaderContainer& Get() {
        static CShaderContainer instance;
        return instance;
    }

    CShaderContainer(const CShaderContainer&) = delete;
    CShaderContainer& operator=(const CShaderContainer&) = delete;
    std::shared_ptr<CShader> GetOrLoad(ID3D11Device* device, const std::wstring& filePath,
        const std::string& vsEntry, const std::string& psEntry, 
        D3D11_INPUT_ELEMENT_DESC* layout, UINT layoutCount);

    void Clear() { m_Shaders.clear(); }

private:
    CShaderContainer() = default;
    ~CShaderContainer() = default;

    std::unordered_map<std::string, std::shared_ptr<CShader>> m_Shaders;
};
