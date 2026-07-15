//----------------------------------------------------------------------------
//-- CUISelectedItemProp: Scene Selected Item Properties Window
//----------------------------------------------------------------------------
#pragma once
#include "CImGuiWindow.h"

class CUISelectedItemProp : public CImGuiWindow {
public:
	CUISelectedItemProp() : CImGuiWindow("Properties") {};
	CUISelectedItemProp(std::string name) : CImGuiWindow(name) {};
	CUISelectedItemProp(const CUISelectedItemProp&) = delete;
	CUISelectedItemProp& operator=(const CUISelectedItemProp&) = delete;

	static CUISelectedItemProp& Get() {
		static CUISelectedItemProp UISelectedItemProp("Properties");
		return UISelectedItemProp;
	}

	void RenderContent() override;
};
