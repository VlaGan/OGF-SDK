//----------------------------------------------------------------------------
//-- CUISceneSettings: general scene settings window for the editor
//----------------------------------------------------------------------------
#pragma once
#include "CImGuiWindow.h"
#include <fontawesome/IconsFontAwesome6.h>

class CUISceneSettings : public CImGuiWindow {
public:
	CUISceneSettings() : CImGuiWindow("Scene Settings") { m_Icon = ICON_FA_SLIDERS; };
	CUISceneSettings(std::string name) : CImGuiWindow(name) { m_Icon = ICON_FA_SLIDERS; };
	CUISceneSettings(const CUISceneSettings&) = delete;
	CUISceneSettings& operator=(const CUISceneSettings&) = delete;

	static CUISceneSettings& Get() {
		static CUISceneSettings UISceneSettings("Scene Settings");
		return UISceneSettings;
	}

	void RenderContent() override;
};
