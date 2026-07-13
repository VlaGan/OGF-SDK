//----------------------------------------------------------------------------
//-- CMesh.cpp
//-- VlaGan: Model unit (mesh)
//----------------------------------------------------------------------------
#include "CMesh.h"
#include "../../_defines.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>

#include "../CRenderer.h"

//-- Constructor / destructor
CMesh::CMesh() {
	m_VertexBuffer = nullptr;
	m_IndexBuffer = nullptr;
	m_VertexCount = 0;
	m_IndexCount = 0;
}

CMesh::~CMesh() { Release(); }

void CMesh::Release() {

    RELEASE(m_VertexBuffer);
    RELEASE(m_IndexBuffer);
	m_VertexCount = 0;
	m_IndexCount = 0;
    
    m_Shader.reset();
    m_ShaderSM.reset();
    m_ShaderGBuffer.reset();

    RELEASE(m_Texture);
    RELEASE(m_Sampler);
}

CMesh::CMesh(CMesh&& other) noexcept {
    m_VertexBuffer = other.m_VertexBuffer;
    m_IndexBuffer = other.m_IndexBuffer;
    m_VertexCount = other.m_VertexCount;
    m_IndexCount = other.m_IndexCount;
    m_Texture = other.m_Texture;
    m_Sampler = other.m_Sampler;
    m_IsTransparent = other.m_IsTransparent;
    m_TextureName = other.m_TextureName;

    m_vVertices = std::move(other.m_vVertices);
    m_vIndices = std::move(other.m_vIndices);

    other.m_VertexBuffer = nullptr;
    other.m_IndexBuffer = nullptr;
    other.m_Texture = nullptr;
    other.m_Sampler = nullptr;
    other.m_VertexCount = 0;
    other.m_IndexCount = 0;
    other.m_IsTransparent = false;

    m_Shader = std::move(other.m_Shader);
    m_ShaderSM = std::move(other.m_ShaderSM);
    m_ShaderGBuffer = std::move(other.m_ShaderGBuffer);
}

CMesh& CMesh::operator=(CMesh&& other) noexcept {
    if (this != &other) {
        Release();

        m_VertexBuffer = other.m_VertexBuffer;
        m_IndexBuffer = other.m_IndexBuffer;
        m_VertexCount = other.m_VertexCount;
        m_IndexCount = other.m_IndexCount;
        m_Texture = other.m_Texture;
        m_Sampler = other.m_Sampler;
        m_IsTransparent = other.m_IsTransparent;
        m_TextureName = other.m_TextureName;

        m_vVertices = std::move(other.m_vVertices);
        m_vIndices = std::move(other.m_vIndices);

        other.m_VertexBuffer = nullptr;
        other.m_IndexBuffer = nullptr;
        other.m_Texture = nullptr;
        other.m_Sampler = nullptr;
        other.m_VertexCount = 0;
        other.m_IndexCount = 0;
        other.m_IsTransparent = false;

        m_Shader = std::move(other.m_Shader);
        m_ShaderSM = std::move(other.m_ShaderSM);
        m_ShaderGBuffer = std::move(other.m_ShaderGBuffer);
    }
    return *this;
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//-- VlaGan: Loading Mesh
//----------------------------------------------------------------------------
bool IsTextureTransparent(const unsigned char* data, int width, int height, int channels)
{
    if (channels < 4) return false;

    for (int i = 0; i < width * height; ++i) {
        int alpha = data[i * 4 + 3];
        if (alpha < 255) return true; // або < threshold
    }
    return false;
}

bool CMesh::Init(ID3D11Device* device, aiMesh* mesh, std::string texture_name) {

    //-- Fill Vertex buffer
    LogMsg("Number of UV channels: [%d]", mesh->GetNumUVChannels());
    for (unsigned i = 0; i < mesh->mNumVertices; ++i)
    {
        Vertex v;
        //-- position
        v.position = {
            mesh->mVertices[i].x,
            mesh->mVertices[i].y,
            mesh->mVertices[i].z
        };

        //-- normal
        v.normal = mesh->HasNormals()
            ? DirectX::XMFLOAT3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z)
            : DirectX::XMFLOAT3(0, 0, 0);

        //-- uv map for texturing
        /*v.texcoord = mesh->HasTextureCoords(0)
            ? DirectX::XMFLOAT2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y)
            : DirectX::XMFLOAT2(0.5f, 0.5f);*/

        //-- blyat this uv mapping
        if (mesh->HasTextureCoords(0))
            v.texcoord = { mesh->mTextureCoords[0][i].x, 1.0f - mesh->mTextureCoords[0][i].y };

        m_vVertices.push_back(v);
    }

    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = UINT(sizeof(Vertex) * m_vVertices.size());
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = m_vVertices.data();

    HRESULT hr = device->CreateBuffer(&bd, &initData, &m_VertexBuffer);
    if (FAILED(hr)) return false;

    m_VertexCount = (UINT)m_vVertices.size();

    //-- Fill Index Buffer
    std::vector<UINT> m_vIndices;
    for (unsigned i = 0; i < mesh->mNumFaces; ++i) {
        const aiFace& face = mesh->mFaces[i];
        for (unsigned j = 0; j < face.mNumIndices; ++j) {
            m_vIndices.push_back(face.mIndices[j]);
        }
    }
    if (!m_vIndices.size())
        return false;

    D3D11_BUFFER_DESC ibd = {};
    ibd.Usage = D3D11_USAGE_DEFAULT;
    ibd.ByteWidth = UINT(sizeof(UINT) * m_vIndices.size());
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA iinit = {};
    iinit.pSysMem = m_vIndices.data();

    HRESULT ihr = device->CreateBuffer(&ibd, &iinit, &m_IndexBuffer);
    if (FAILED(ihr)) return false;
    m_IndexCount = (UINT)m_vIndices.size();

    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, position), D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, normal),   D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, offsetof(Vertex, texcoord), D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "BLENDINDICES", 0, DXGI_FORMAT_R32G32B32A32_SINT, 0, offsetof(Vertex, boneIDs), D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "BLENDWEIGHT",  0, DXGI_FORMAT_R32G32B32A32_FLOAT,0, offsetof(Vertex, weights), D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    //m_Shader.LoadFromFile(device, L"appdata/shaders/skinned.hlsl", "VSMain", "PSMain", layout, ARRAYSIZE(layout));

    m_Shader = CShaderContainer::Get().GetOrLoad(device, L"appdata/shaders/skinned.hlsl", "VSMain", "PSMain", layout, ARRAYSIZE(layout));
    m_ShaderSM = CShaderContainer::Get().GetOrLoad(device, L"appdata/shaders/skinned_shadow_map.hlsl", "VSMain", "PSMain", layout, ARRAYSIZE(layout));
    m_ShaderGBuffer = CShaderContainer::Get().GetOrLoad(device, L"appdata/shaders/GBuffer.hlsl", "VSMain", "PSMain", layout, ARRAYSIZE(layout));

    //-- simple example texture loading (later should be CTexture class or CTexture container manager)
    int x, y, channels_in_file;
    stbi_info(texture_name.c_str(), &x, &y, &channels_in_file);
    LogMsg("Texture [%s] has %d channels in file", texture_name.c_str(), channels_in_file);
    int w, h, channels;
    unsigned char* data = stbi_load(texture_name.c_str(), &w, &h, &channels, STBI_rgb_alpha);
    if (!data) {
        LogMsg("!CMesh::Init: cant load texture[%s] fall back to default[image.png]", texture_name.c_str());
        data = stbi_load("appdata/textures/image.png", &w, &h, &channels, STBI_rgb_alpha);
    }
    m_TextureName = texture_name;

    m_IsTransparent = IsTextureTransparent(data, w, h, channels);
    if (m_IsTransparent)
        LogMsg("!!!Transparent texture found [%s]", texture_name.c_str());


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

    ID3D11Texture2D* texture = nullptr;
     hr = device->CreateTexture2D(&desc, &initTex, &texture);
    if (FAILED(hr)) {
        MessageBox(nullptr, L"CreateTexture2D failed", L"Error", MB_OK);
        return false;
    }

    hr = device->CreateShaderResourceView(texture, nullptr, &m_Texture);
    if (FAILED(hr)) {
        MessageBox(nullptr, L"CreateShaderResourceView failed", L"Error", MB_OK);
        return false;
    }
    if(texture)
    texture->Release();
    stbi_image_free(data);

    // Create sampler
    D3D11_SAMPLER_DESC sampDesc = {};
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;

    device->CreateSamplerState(&sampDesc, &m_Sampler);

    LogMsg("-CMesh::Init: succesfully loaded mesh!");

    LogMsg("Mesh info: verticies [%d], indicies [%d]", m_VertexCount, m_IndexCount);

	return true;
}

//-- pos, normal and uvs defined in CMesh::Init, but bones and weight for skinning in CModel
//-- so we need to update buffer after their filling
void CMesh::UpdateVertexBuffer(ID3D11Device* device) {
    if (m_VertexBuffer) {
        m_VertexBuffer->Release();
        m_VertexBuffer = nullptr;
    }

    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = UINT(sizeof(Vertex) * m_vVertices.size());
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = m_vVertices.data();

    HRESULT hr = device->CreateBuffer(&bd, &initData, &m_VertexBuffer);

    m_VertexCount = (UINT)m_vVertices.size();

}

//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//-- VlaGan: Mesh Rendering
//----------------------------------------------------------------------------
void CMesh::Render(ID3D11DeviceContext* context) {
    //-- texture to shader
    m_Shader.get()->Bind(context);

    context->PSSetShaderResources(0, 1, &m_Texture);
    context->PSSetSamplers(0, 1, &m_Sampler);

    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, &m_VertexBuffer, &stride, &offset);
    context->IASetIndexBuffer(m_IndexBuffer, DXGI_FORMAT_R32_UINT, 0);

    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->DrawIndexed(m_IndexCount, 0, 0);
}

void CMesh::RenderSM(ID3D11DeviceContext* context) {
    m_ShaderSM.get()->Bind(context);
    context->PSSetShader(nullptr, nullptr, 0);

    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, &m_VertexBuffer, &stride, &offset);
    context->IASetIndexBuffer(m_IndexBuffer, DXGI_FORMAT_R32_UINT, 0);

    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->DrawIndexed(m_IndexCount, 0, 0);
}

//-- render mesh gbuffer
void CMesh::RenderGBuffer(ID3D11DeviceContext* context) {
    m_ShaderGBuffer.get()->Bind(context);
    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, &m_VertexBuffer, &stride, &offset);
    context->IASetIndexBuffer(m_IndexBuffer, DXGI_FORMAT_R32_UINT, 0);

    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->DrawIndexed(m_IndexCount, 0, 0);
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//-- VlaGan: Set custom mesh shader (later update better)
//----------------------------------------------------------------------------
void CMesh::SetShader(ID3D11Device* device, const std::wstring& filePath) {
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, position), D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, normal),   D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, offsetof(Vertex, texcoord), D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "BLENDINDICES", 0, DXGI_FORMAT_R32G32B32A32_SINT, 0, offsetof(Vertex, boneIDs), D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "BLENDWEIGHT",  0, DXGI_FORMAT_R32G32B32A32_FLOAT,0, offsetof(Vertex, weights), D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    m_Shader = CShaderContainer::Get().GetOrLoad(device, filePath, "VSMain", "PSMain", layout, ARRAYSIZE(layout));
}

void CMesh::SetShadowShader(ID3D11Device* device, const std::wstring& filePath) {
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, position), D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, normal),   D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, offsetof(Vertex, texcoord), D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "BLENDINDICES", 0, DXGI_FORMAT_R32G32B32A32_SINT, 0, offsetof(Vertex, boneIDs), D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "BLENDWEIGHT",  0, DXGI_FORMAT_R32G32B32A32_FLOAT,0, offsetof(Vertex, weights), D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    m_ShaderSM = CShaderContainer::Get().GetOrLoad(device, filePath, "VSMain", "PSMain", layout, ARRAYSIZE(layout));
}
//----------------------------------------------------------------------------
