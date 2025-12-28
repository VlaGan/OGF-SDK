//----------------------------------------------------------------------------
//-- CRenderer - main rendering class
//----------------------------------------------------------------------------
#pragma once
#include <d3d11.h>
#include "Templates/RenderTarget.h"

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

	float m_ClearColor[4]{ 1.f, 0.0f, 0.0f, 1.0f };

//private:
	CRenderTarget m_mnRT;
};