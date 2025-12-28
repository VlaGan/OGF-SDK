//----------------------------------------------------------------------------
//-- RenderTarget.cpp
//-- VlaGan: Basic rendertarget class
//----------------------------------------------------------------------------
#include "RenderTarget.h"
#include <stdint.h>
#include "../../_defines.h"

CRenderTarget::CRenderTarget() {}

CRenderTarget::~CRenderTarget() {
	Release();
}

bool CRenderTarget::Create(ID3D11Device* device, UINT width, UINT height, DXGI_FORMAT tformat) {
	Release();

	m_Width = width;
	m_Height = height;
	m_format = tformat;

	// (RGBA, world-space normal)
	D3D11_TEXTURE2D_DESC tDesc = {};
	tDesc.Width = m_Width;
	tDesc.Height = m_Height;
	tDesc.MipLevels = 1;
	tDesc.ArraySize = 1;
	tDesc.Format = tformat;
	tDesc.SampleDesc.Count = 1;
	tDesc.Usage = D3D11_USAGE_DEFAULT;
	tDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

	//D3D11_SUBRESOURCE_DATA (nullptr downside)
	device->CreateTexture2D(&tDesc, nullptr, &m_Texture);

	//D3D11_RENDER_TARGET_VIEW_DESC 
	D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = tDesc.Format;
	rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	device->CreateRenderTargetView(m_Texture, &rtvDesc, &m_RTV);

	//D3D11_SHADER_RESOURCE_VIEW_DESC
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = tDesc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	device->CreateShaderResourceView(m_Texture, &srvDesc, &m_SRV);

	return true;
}

//-- release resources
void CRenderTarget::Release() {
	RELEASE(m_Texture);
	RELEASE(m_RTV);
	RELEASE(m_SRV);
}
