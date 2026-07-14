//----------------------------------------------------------------------------
//-- CUISceneSettings: general scene settings window for the editor
//----------------------------------------------------------------------------
#pragma once
#include "CImGuiWindow.h"

class CUISceneSettings : public CImGuiWindow {
public:
	CUISceneSettings() : CImGuiWindow("Scene Settings") {};
	CUISceneSettings(std::string name) : CImGuiWindow(name) {};
	CUISceneSettings(const CUISceneSettings&) = delete;
	CUISceneSettings& operator=(const CUISceneSettings&) = delete;

	static CUISceneSettings& Get() {
		static CUISceneSettings UISceneSettings("Scene Settings");
		return UISceneSettings;
	}

	void RenderContent() override;
};
