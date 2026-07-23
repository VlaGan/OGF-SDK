//----------------------------------------------------------------------------
//-- CMesh.cpp
//-- VlaGan: Model unit (mesh)
//----------------------------------------------------------------------------
#include "CMesh.h"
#include "../../_defines.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
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
    m_ShaderOutline.reset();

    m_Texture.Release();
    RELEASE(m_Sampler);
}

CMesh::CMesh(CMesh&& other) noexcept {
    m_VertexBuffer = other.m_VertexBuffer;
    m_IndexBuffer = other.m_IndexBuffer;
    m_VertexCount = other.m_VertexCount;
    m_IndexCount = other.m_IndexCount;
    m_Sampler = other.m_Sampler;
    m_ShaderName = other.m_ShaderName;

    m_vVertices = std::move(other.m_vVertices);
    m_vIndices = std::move(other.m_vIndices);

    other.m_VertexBuffer = nullptr;
    other.m_IndexBuffer = nullptr;
    other.m_Sampler = nullptr;
    other.m_VertexCount = 0;
    other.m_IndexCount = 0;

    m_Shader = std::move(other.m_Shader);
    m_ShaderSM = std::move(other.m_ShaderSM);
    m_ShaderGBuffer = std::move(other.m_ShaderGBuffer);
    m_ShaderOutline = std::move(other.m_ShaderOutline);

    m_Texture = std::move(other.m_Texture);
}

CMesh& CMesh::operator=(CMesh&& other) noexcept {
    if (this != &other) {
        Release();

        m_VertexBuffer = other.m_VertexBuffer;
        m_IndexBuffer = other.m_IndexBuffer;
        m_VertexCount = other.m_VertexCount;
        m_IndexCount = other.m_IndexCount;
        m_Sampler = other.m_Sampler;
        m_ShaderName = other.m_ShaderName;

        m_vVertices = std::move(other.m_vVertices);
        m_vIndices = std::move(other.m_vIndices);

        other.m_VertexBuffer = nullptr;
        other.m_IndexBuffer = nullptr;
        other.m_Sampler = nullptr;
        other.m_VertexCount = 0;
        other.m_IndexCount = 0;

        m_Shader = std::move(other.m_Shader);
        m_ShaderSM = std::move(other.m_ShaderSM);
        m_ShaderGBuffer = std::move(other.m_ShaderGBuffer);
        m_ShaderOutline = std::move(other.m_ShaderOutline);

        m_Texture = std::move(other.m_Texture);
    }
    return *this;
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//-- VlaGan: Loading Mesh
//----------------------------------------------------------------------------
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

    CreateDefaultShaders(device);

    if (!LoadTextureResource(device, texture_name))
        return false;

    LogMsg("-CMesh::Init: succesfully loaded mesh!");

    LogMsg("Mesh info: verticies [%d], indicies [%d]", m_VertexCount, m_IndexCount);

	return true;
}

//----------------------------------------------------------------------------
//-- shared helpers (used by Init() and InitFromRaw())
//----------------------------------------------------------------------------
void CMesh::CreateDefaultShaders(ID3D11Device* device) {
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, position), D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, normal),   D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, offsetof(Vertex, texcoord), D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "BLENDINDICES", 0, DXGI_FORMAT_R32G32B32A32_SINT, 0, offsetof(Vertex, boneIDs), D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "BLENDWEIGHT",  0, DXGI_FORMAT_R32G32B32A32_FLOAT,0, offsetof(Vertex, weights), D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    m_Shader = CShaderContainer::Get().GetOrLoad(device, L"appdata/shaders/skinned.hlsl", "VSMain", "PSMain", layout, ARRAYSIZE(layout));
    m_ShaderSM = CShaderContainer::Get().GetOrLoad(device, L"appdata/shaders/skinned_shadow_map.hlsl", "VSMain", "PSMain", layout, ARRAYSIZE(layout));
    m_ShaderGBuffer = CShaderContainer::Get().GetOrLoad(device, L"appdata/shaders/GBuffer.hlsl", "VSMain", "PSMain", layout, ARRAYSIZE(layout));
    m_ShaderOutline = CShaderContainer::Get().GetOrLoad(device, L"appdata/shaders/skinned_outline.hlsl", "VSMain", "PSMain", layout, ARRAYSIZE(layout));
}

bool CMesh::LoadTextureResource(ID3D11Device* device, const std::string& texture_name) {

    if (!m_Texture.LoadFromFile(device, texture_name))
        return false;

    D3D11_SAMPLER_DESC sampDesc = {};
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;

    device->CreateSamplerState(&sampDesc, &m_Sampler);
    return true;
}

//----------------------------------------------------------------------------
//-- native (non-Assimp) geometry loading - used by COgfLoader for .ogf files
//----------------------------------------------------------------------------
bool CMesh::InitFromRaw(ID3D11Device* device, std::vector<Vertex> vertices, std::vector<UINT> indices, std::string texture_name, std::string shader_name) {
    if (vertices.empty() || indices.empty()) {
        LogMsg(eLogLevel::ERR, "!CMesh::InitFromRaw: empty vertex/index data for texture[%s]", texture_name.c_str());
        return false;
    }

    m_ShaderName = std::move(shader_name);
    m_vVertices = std::move(vertices);
    m_vIndices = std::move(indices);

    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = UINT(sizeof(Vertex) * m_vVertices.size());
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = m_vVertices.data();

    HRESULT hr = device->CreateBuffer(&bd, &initData, &m_VertexBuffer);
    if (FAILED(hr)) return false;

    m_VertexCount = (UINT)m_vVertices.size();

    D3D11_BUFFER_DESC ibd = {};
    ibd.Usage = D3D11_USAGE_DEFAULT;
    ibd.ByteWidth = UINT(sizeof(UINT) * m_vIndices.size());
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA iinit = {};
    iinit.pSysMem = m_vIndices.data();

    HRESULT ihr = device->CreateBuffer(&ibd, &iinit, &m_IndexBuffer);
    if (FAILED(ihr)) return false;
    m_IndexCount = (UINT)m_vIndices.size();

    CreateDefaultShaders(device);

    if (!LoadTextureResource(device, texture_name))
        return false;

    LogMsg(eLogLevel::WARNING, "-CMesh::InitFromRaw: succesfully loaded mesh!");
    LogMsg(eLogLevel::WARNING, "Mesh info: verticies [%d], indicies [%d]", m_VertexCount, m_IndexCount);

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

    ID3D11ShaderResourceView* srv = m_Texture.GetSRV();
    if(srv)
        context->PSSetShaderResources(0, 1, &srv);

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

//-- Blender-style selection outline (inverted-hull pass, no texture needed)
void CMesh::RenderOutline(ID3D11DeviceContext* context) {
    m_ShaderOutline.get()->Bind(context);

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
