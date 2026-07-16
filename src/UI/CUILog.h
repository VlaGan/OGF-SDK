//----------------------------------------------------------------------------
//-- CUILog: Log window for ImGui Docking
//----------------------------------------------------------------------------
#pragma once
#include "CImGuiWindow.h"
#include <fontawesome/IconsFontAwesome6.h>

class CUILog: public CImGuiWindow {
public:
	CUILog() : CUILog("Log") {};
	CUILog(std::string name) : CImGuiWindow(name) { m_Icon = ICON_FA_TERMINAL; };
	CUILog(const CUILog&) = delete;
	CUILog& operator=(const CUILog&) = delete;

	static CUILog& Get() {
		static CUILog UILog("Log");
		return UILog;
	}

	void RenderContent() override;

private:
	bool m_AutoScroll{ true };
	bool m_ShowInfo{ true };
	bool m_ShowWarning{ true };
	bool m_ShowError{ true };
	bool m_ShowDebug{ true };
};
