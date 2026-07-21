//----------------------------------------------------------------------------
//-- CRenderer - main rendering class
//----------------------------------------------------------------------------
#pragma once
#include <d3d11.h>
#include "Templates/RenderTarget.h"
#include "Templates/ConstantBuffer.h"
#include "_render_structs.h"
#include "CDebugRenderer.h"

#include "Model/CModel.h"

class CRenderer {
public:
	CRenderer();
	~CRenderer();

	CRenderer(const CRenderer&) = delete;
	CRenderer& operator=(const CRenderer&) = delete;

	static CRenderer& Get() {
		static CRenderer CRender;
		return CRender;
	}

	bool Init(UINT dwW, UINT dwH);

	void Render();
	void Resize(UINT dwW, UINT dwH);

	float m_ClearColor[4]{ 0.5f, 0.5f, 0.5f, 1.0f };

	CRenderTarget* GetMainRT() { return &m_mnRT; }
	CDebugRenderer* DebugRenderer() { return &m_debugRenderer; }

	CConstantBuffer m_ConstantBuffer;
	CConstantBuffer m_LightBuffer;


	DirectX::XMMATRIX m_View;
	DirectX::XMMATRIX m_Projection;
	DirectX::XMMATRIX m_ViewProj;

	DirectX::XMMATRIX m_LightView;
	DirectX::XMMATRIX m_LightProj;
	DirectX::XMMATRIX m_LightViewProj;

	ULONGLONG timePrev{};
	ULONGLONG timeCur{};

	float dwWidth{}, dwHeight{};
private:

	ID3D11Texture2D* m_DepthStencilBuffer{};
	ID3D11DepthStencilView* m_DepthStencilView{};


	CDebugRenderer m_debugRenderer;

	ID3D11RasterizerState* m_RasterState{};
	ID3D11RasterizerState* m_RasterStateWireframe{};
	ID3D11DepthStencilState* m_DepthStencilState{};
	ID3D11DepthStencilState* m_DepthStencilTransparent{};
	ID3D11BlendState* m_AlphaBlendState{};

	CRenderTarget m_mnRT;
};