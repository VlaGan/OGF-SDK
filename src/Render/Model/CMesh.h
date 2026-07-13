//----------------------------------------------------------------------------
//-- CMesh.h
//-- VlaGan: Model unit (mesh) .header
//----------------------------------------------------------------------------
#pragma once

#include <d3d11.h>
#include <vector>
#include "../Templates/CShader.h"
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
    void Render(ID3D11DeviceContext* context);
    void RenderSM(ID3D11DeviceContext* context);
    void RenderGBuffer(ID3D11DeviceContext* context);

    void Release();

    void SetShader(ID3D11Device* device, const std::wstring& filePath);
    void SetShadowShader(ID3D11Device* device, const std::wstring& filePath);

    void UpdateVertexBuffer(ID3D11Device* device);

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
    ID3D11ShaderResourceView* m_Texture{};
    //ID3D11ShaderResourceView* m_TextureNM{};
    ID3D11SamplerState* m_Sampler{};

    bool m_IsTransparent{};
    std::string m_TextureName{};
};