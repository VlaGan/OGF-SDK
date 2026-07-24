//----------------------------------------------------------------------------
//-- CUISelectedItemProp: Scene Selected Item Properties Window
//----------------------------------------------------------------------------
#pragma once
#include "CImGuiWindow.h"
#include <fontawesome/IconsFontAwesome6.h>

class CUISelectedItemProp : public CImGuiWindow {
public:
	CUISelectedItemProp() : CImGuiWindow("Properties") { m_Icon = ICON_FA_CUBE; };
	CUISelectedItemProp(std::string name) : CImGuiWindow(name) { m_Icon = ICON_FA_CUBE; };
	CUISelectedItemProp(const CUISelectedItemProp&) = delete;
	CUISelectedItemProp& operator=(const CUISelectedItemProp&) = delete;

	static CUISelectedItemProp& Get() {
		static CUISelectedItemProp UISelectedItemProp("Properties");
		return UISelectedItemProp;
	}

	void RenderContent() override;

	//-- Model data tabs
	void RenderModelTransform();
	void RenderModelInfo();
	void RenderMeshesData();
	void RenderSkeletonData();
	void RenderMotionsData();
};
