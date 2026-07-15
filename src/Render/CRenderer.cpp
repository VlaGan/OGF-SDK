//----------------------------------------------------------------------------
//-- CRenderer - main rendering class
//----------------------------------------------------------------------------
#include "CRenderer.h"
#include "CHW.h"
#include "../_defines.h"
#include <d3dcompiler.h>
#include "../Core/CCamera.h"

#include "CScene.h"

extern CCamera m_Camera; //-- peredelat potom

//-- Camera parameterss (cut this shit later)
float fFov = 1.f;
DirectX::XMFLOAT3 camPos = DirectX::XMFLOAT3(0.0f, 1.0f, -5.0f);
DirectX::XMFLOAT4 camAt = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
DirectX::XMFLOAT4 camUp = DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 0.0f);

//-- Blinn-Phong like
DirectX::XMFLOAT3 lightDir = DirectX::XMFLOAT3(-0.5f, -1.0f, -0.3f);
DirectX::XMFLOAT4 lightColor = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
float specularPower = 32.0f;

CRenderer::CRenderer() {
    m_View = DirectX::XMMatrixIdentity();
    m_Projection = DirectX::XMMatrixIdentity();
    m_ViewProj = DirectX::XMMatrixIdentity();
    m_LightView = DirectX::XMMatrixIdentity();
    m_LightProj = DirectX::XMMatrixIdentity();
    m_LightViewProj = DirectX::XMMatrixIdentity();
}


CRenderer::~CRenderer() {
	m_mnRT.Release();

	RELEASE(m_DepthStencilBuffer);
	RELEASE(m_DepthStencilView);
	RELEASE(m_RasterState);
    RELEASE(m_RasterStateWireframe);
    RELEASE(m_DepthStencilState);
    RELEASE(m_DepthStencilTransparent);
    RELEASE(m_AlphaBlendState);
}

void CRenderer::Resize(UINT dwW, UINT dwH) {
    dwWidth = (float)dwW;
    dwHeight = (float)dwH;

	CHW& hw = CHW::Get();
	m_mnRT.Create(hw.m_Device, dwW, dwH, DXGI_FORMAT_R8G8B8A8_UNORM);
	
    //-- Depth buffer
	RELEASE(m_DepthStencilBuffer);
	RELEASE(m_DepthStencilView);

	D3D11_TEXTURE2D_DESC depthDesc = {};
	depthDesc.Width = dwW;
	depthDesc.Height = dwH;
	depthDesc.MipLevels = 1;
	depthDesc.ArraySize = 1;
	depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthDesc.SampleDesc.Count = 1;
	depthDesc.SampleDesc.Quality = 0;
	depthDesc.Usage = D3D11_USAGE_DEFAULT;
	depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	HRESULT hr;
	hr = hw.m_Device->CreateTexture2D(&depthDesc, nullptr, &m_DepthStencilBuffer);
	if (FAILED(hr)) return;
	hr = hw.m_Device->CreateDepthStencilView(m_DepthStencilBuffer, nullptr, &m_DepthStencilView);
	if (FAILED(hr)) return;

    D3D11_VIEWPORT vp = {};
    vp.Width = (FLOAT)dwWidth;
    vp.Height = (FLOAT)dwHeight;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    hw.m_Context->RSSetViewports(1, &vp);
}

bool CRenderer::Init(UINT dwW, UINT dwH) {
	CHW& hw = CHW::Get();

	m_mnRT.Create(hw.m_Device, dwW, dwH, DXGI_FORMAT_R8G8B8A8_UNORM);
    m_ConstantBuffer.Create(hw.m_Device, sizeof(MatrixBuffer));

    //-- Depth buffeer
    D3D11_TEXTURE2D_DESC depthDesc = {};
    depthDesc.Width = dwW;  //-- Should be equal to windows wnd size
    depthDesc.Height = dwH;
    depthDesc.MipLevels = 1;
    depthDesc.ArraySize = 1;
    depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; //-- standart format
    depthDesc.SampleDesc.Count = 1;
    depthDesc.SampleDesc.Quality = 0;
    depthDesc.Usage = D3D11_USAGE_DEFAULT;
    depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    depthDesc.CPUAccessFlags = 0;
    depthDesc.MiscFlags = 0;

    HRESULT hr;
    hr = hw.m_Device->CreateTexture2D(&depthDesc, nullptr, &m_DepthStencilBuffer);
    if (FAILED(hr)) return false;

    hr = hw.m_Device->CreateDepthStencilView(m_DepthStencilBuffer, nullptr, &m_DepthStencilView);
    if (FAILED(hr)) return false;

    //-- Setup camera View and Projection
    m_View = DirectX::XMMatrixLookAtLH(
        DirectX::XMVectorSet(VPUSH3(camPos), 0.0f),  // (eye)
        DirectX::XMVectorSet(VPUSH4(camAt)),   // (at)
        DirectX::XMVectorSet(VPUSH4(camUp))    // (up)
    );


    m_Projection = DirectX::XMMatrixPerspectiveFovLH(
        fFov, (float)dwW / (float)dwH, m_Camera.zNear, m_Camera.zFar
    );

    m_debugRenderer.Initialize(hw.m_Device);

    //-- rasterizer state to disable backface culling
    D3D11_RASTERIZER_DESC rasterDesc = {};
    rasterDesc.CullMode = D3D11_CULL_NONE;
    rasterDesc.FillMode = D3D11_FILL_SOLID;
    rasterDesc.FrontCounterClockwise = false;
    hw.m_Device->CreateRasterizerState(&rasterDesc, &m_RasterState);

    rasterDesc.FillMode = D3D11_FILL_WIREFRAME;
    hw.m_Device->CreateRasterizerState(&rasterDesc, &m_RasterStateWireframe);


    D3D11_DEPTH_STENCIL_DESC dsDesc = {};
    dsDesc.DepthEnable = true;
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    dsDesc.DepthFunc = D3D11_COMPARISON_LESS;

    hr = hw.m_Device->CreateDepthStencilState(&dsDesc, &m_DepthStencilState);
    if (FAILED(hr)) return false;

    //-- transparent textures
    D3D11_DEPTH_STENCIL_DESC transparentDesc = {};
    transparentDesc.DepthEnable = true;
    transparentDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    transparentDesc.DepthFunc = D3D11_COMPARISON_LESS;
    hw.m_Device->CreateDepthStencilState(&transparentDesc, &m_DepthStencilTransparent);

    //-- alpha blending
    D3D11_BLEND_DESC blendStateDesc;
    ZeroMemory(&blendStateDesc, sizeof(D3D11_BLEND_DESC));
    blendStateDesc.AlphaToCoverageEnable = FALSE;
    blendStateDesc.IndependentBlendEnable = FALSE;
    blendStateDesc.RenderTarget[0].BlendEnable = TRUE;
    blendStateDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendStateDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendStateDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendStateDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendStateDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
    blendStateDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_MAX;
    blendStateDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    hw.m_Device->CreateBlendState(&blendStateDesc, &m_AlphaBlendState);

    D3D11_VIEWPORT vp = {};
    vp.Width = (FLOAT)dwWidth;
    vp.Height = (FLOAT)dwHeight;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    hw.m_Context->RSSetViewports(1, &vp);

    LogMsg(eLogLevel::WARNING, "CRenderer::Init::Success [%f, %f]", dwWidth, dwHeight);

	return true;
}

// ground grid
void DebugDrawGrid(ID3D11DeviceContext* context, CDebugRenderer& debug, float size, float step) {
    DirectX::XMFLOAT4 gridColor(0.8f, 0.8f, 0.8f, 1.0f);
    DirectX::XMFLOAT4 red(1.0f, 0.0f, 0.0f, 1.0f);
    DirectX::XMFLOAT4 green(0.0f, 1.0f, 0.0f, 1.0f);

    for (float x = -size; x <= size; x += step) {
        debug.DrawLine(context,
            DirectX::XMFLOAT3(x, 0, -size),
            DirectX::XMFLOAT3(x, 0, size),
            !x ? red : gridColor);
    }
    for (float z = -size; z <= size; z += step) {
        debug.DrawLine(context,
            DirectX::XMFLOAT3(-size, 0, z),
            DirectX::XMFLOAT3(size, 0, z),
            !z ? green: gridColor);
    }
}


void DebugDrawSkeleton(ID3D11DeviceContext* context, CDebugRenderer& debug, CBoneInstance* pBi) {
    if (!pBi)
        return;

    DirectX::XMFLOAT4 linecolor(0.0f, 1.0f, 0.0f, 1.0f);
    DirectX::XMFLOAT4 boxcolor(1.0f, 1.0f, 0.0f, 1.0f);

    DirectX::XMFLOAT3 par_pos;
    DirectX::XMStoreFloat3(&par_pos, pBi->mGlobalTransform.r[3]);
    debug.DrawCube(context, par_pos, 0.005f, boxcolor);

    for (auto& ch : pBi->m_vChilds)
    {
        DirectX::XMFLOAT3 ch_pos;
        DirectX::XMStoreFloat3(&ch_pos, ch->mGlobalTransform.r[3]);
        debug.DrawLine(context, par_pos, ch_pos, linecolor);
        DebugDrawSkeleton(context, debug, ch);

    }
}


void CRenderer::Render() {
	CHW& hw = CHW::Get();

    // Update delta time
    timeCur = GetTickCount64();
    if (timePrev == 0) timePrev = timeCur;

    float dt = (timeCur - timePrev) / 1000.0f;
    timePrev = timeCur;

    //-- update camera
    m_Camera.Update(dt);

	CScene& scene = CScene::Get();
	for (auto& model : scene.GetModels())
		model->Update(dt);

    m_View = m_Camera.GetViewMatrix();
    m_Projection = m_Camera.GetProjectionMatrix((float)dwWidth / (float)dwHeight);
    m_ViewProj = DirectX::XMMatrixMultiply(m_View, m_Projection);

	//-- Lets draw to main render target
	hw.m_Context->OMSetRenderTargets(1, &m_mnRT.m_RTV, m_DepthStencilView);
	hw.m_Context->ClearRenderTargetView(m_mnRT.m_RTV, m_ClearColor);
    hw.m_Context->ClearDepthStencilView(m_DepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

    //-- set rasterizer state to disable backface culling (vroid models shit)
	if (scene.m_bWireframe)
		hw.m_Context->RSSetState(m_RasterStateWireframe);
	else
        hw.m_Context->RSSetState(m_RasterState);

    //-- Debug grid
    m_debugRenderer.SetRenderPhase(ePhaseBeforeScene);

    if(scene.m_bDrawGrid)
        DebugDrawGrid(hw.m_Context, m_debugRenderer, 10, 1);
    
    m_debugRenderer.Render(hw.m_Context, m_ViewProj, true);

    //-- Light data setup
    LightCB lightData = { lightDir, 0.0f, lightColor,  camPos , specularPower };
    m_LightBuffer.PSSet(hw.m_Context, 1);
    m_LightBuffer.Update(hw.m_Context, 0, nullptr, &lightData, 0, 0);
    
    //-- simple models rendering
    hw.m_Context->OMSetDepthStencilState(m_DepthStencilState, 1);
    hw.m_Context->OMSetBlendState(nullptr, 0, 0xffffffff);

    for (auto& model : scene.GetModels())
        model->Render(hw.m_Context);

    float blendFactor[4] = { 0.f, 0.f, 0.f, 0.f };
    hw.m_Context->OMSetDepthStencilState(m_DepthStencilTransparent, 1);
    hw.m_Context->OMSetBlendState(m_AlphaBlendState, blendFactor, 0xffffffff);
    for (auto& model : scene.GetModels())
        model->Render(hw.m_Context, true);

    hw.m_Context->OMSetDepthStencilState(nullptr, 1);
    hw.m_Context->OMSetBlendState(nullptr, 0, 0xffffffff);


    m_debugRenderer.SetRenderPhase(ePhaseAfterScene);
    if (scene.m_bDrawSkeleton)
    for (auto& model : scene.GetModels())
        if(model->m_Skeleton.m_BoneCount)
            DebugDrawSkeleton(hw.m_Context, m_debugRenderer, model->m_Skeleton.GetRootBone()); //-- Skeleton debug
    m_debugRenderer.Render(hw.m_Context, m_ViewProj, true);

    /*
    m_debugRenderer.SetRenderPhase(ePhaseAfterScene);
	for (auto& model : scene.GetModels())
		if (model->m_Skeleton.m_BoneCount)
        m_debugRenderer.DrawBoneAxis(hw.m_Context, *model->m_Skeleton.GetRootBone(), 0.5f);

    m_debugRenderer.Render(hw.m_Context, m_ViewProj, true);*/
}
