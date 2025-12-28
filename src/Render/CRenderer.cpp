//----------------------------------------------------------------------------
//-- CRenderer - main rendering class
//----------------------------------------------------------------------------
#include "CRenderer.h"
#include "CHW.h"
#include "../_defines.h"
#include <d3dcompiler.h>

CRenderer::CRenderer() {
}

CRenderer::~CRenderer() {
	m_mnRT.Release();
}

bool CRenderer::Init(UINT dwW, UINT dwH) {
	CHW& hw = CHW::Get();

	m_mnRT.Create(hw.m_Device, dwW, dwH, DXGI_FORMAT_R8G8B8A8_UNORM);
	
	return true;
}

void CRenderer::Render() {
	CHW& hw = CHW::Get();

	hw.m_Context->OMSetRenderTargets(1, &m_mnRT.m_RTV, nullptr);
	hw.m_Context->ClearRenderTargetView(m_mnRT.m_RTV, m_ClearColor);
}