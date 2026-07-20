//----------------------------------------------------------------------------
//-- CMesh.h
//-- VlaGan: Model unit (mesh) .header
//----------------------------------------------------------------------------
#pragma once

#include <d3d11.h>
#include <vector>
#include "../Templates/CShader.h"
#include "../Templates/CTexture.h"
#include "../_render_structs.h"
#include "../CShaderContainer.h"

struct aiMesh;

class CMesh {
public:
    CMesh();
    ~CMesh();

    CMesh(const CMesh&) = delete;
    CMesh& operator=(const CMesh&) = delete;

    CMesh(CMesh&& other) noexcept;
    CMesh& operator=(CMesh&& other) noexcept;

    bool Init(ID3D11Device* device, aiMesh* mesh, std::string texture_name);

    //-- Same as Init(), but the vertex/index data is already prepared (e.g. by
    //-- COgfLoader when reading a native .ogf file) instead of coming from an
    //-- Assimp aiMesh. `vertices`/`indices` are consumed (moved from).
    //-- `shader_name` is the X-Ray shader name from OGF_TEXTURE (informational
    //-- only for now - not used to pick an actual HLSL shader, see m_ShaderName).
    bool InitFromRaw(ID3D11Device* device, std::vector<Vertex> vertices, std::vector<UINT> indices, std::string texture_name, std::string shader_name = "");

    void Render(ID3D11DeviceContext* context);
    void RenderSM(ID3D11DeviceContext* context);
    void RenderGBuffer(ID3D11DeviceContext* context);

    void Release();

    void SetShader(ID3D11Device* device, const std::wstring& filePath);
    void SetShadowShader(ID3D11Device* device, const std::wstring& filePath);

    void UpdateVertexBuffer(ID3D11Device* device);

public:
    bool IsTextureTransparent() { return m_Texture.IsTextureTransparent(); }
    std::string GetTextureName() { return m_Texture.GetFileName(); }

private:
    //-- shared by Init() and InitFromRaw(): loads `texture_name` (falls back to
    //-- appdata/textures/image.png on failure) into m_Texture/m_Sampler and
    //-- fills m_IsTransparent/m_TextureName.
    bool LoadTextureResource(ID3D11Device* device, const std::string& texture_name);

    //-- shared by Init() and InitFromRaw(): creates the skinned/GBuffer/shadow shaders
    void CreateDefaultShaders(ID3D11Device* device);

public:
    //-- Index/Vertex Buffer and etc.
    ID3D11Buffer* m_VertexBuffer;
    ID3D11Buffer* m_IndexBuffer;
    UINT m_VertexCount = 0;
    UINT m_IndexCount = 0;

    std::vector<Vertex> m_vVertices;
    std::vector<UINT> m_vIndices;

    //-- Shaders
    std::shared_ptr<CShader> m_Shader;
    std::shared_ptr<CShader> m_ShaderSM;
    std::shared_ptr<CShader> m_ShaderGBuffer;

    //-- Textures
    CTexture m_Texture;
    ID3D11SamplerState* m_Sampler{};

    //-- X-Ray shader name (OGF_TEXTURE's 2nd string, like "models\\model")
    std::string m_ShaderName{};
};