//----------------------------------------------------------------------------
//-- CUIProperties: Scene Selected Item Properties Window
//----------------------------------------------------------------------------
#pragma once
#include "CImGuiWindow.h"
#include <fontawesome/IconsFontAwesome6.h>

class CUIProperties : public CImGuiWindow {
public:
	CUIProperties() : CImGuiWindow("Properties") { m_Icon = ICON_FA_CUBE; };
	CUIProperties(std::string name) : CImGuiWindow(name) { m_Icon = ICON_FA_CUBE; };
	CUIProperties(const CUIProperties&) = delete;
	CUIProperties& operator=(const CUIProperties&) = delete;

	static CUIProperties& Get() {
		static CUIProperties UIProperties("Properties");
		return UIProperties;
	}

	void RenderContent() override;

	//-- Model data tabs
	void RenderModelTransform();
	void RenderModelInfo();
	void RenderMeshesData();
	void RenderSkeletonData();
	void RenderMotionsData();
};
