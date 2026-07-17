//----------------------------------------------------------------------------
//-- CUIViewPort: Viewport window for ImGui Docking
//----------------------------------------------------------------------------
#pragma once
#include "CImGuiWindow.h"
#include <fontawesome/IconsFontAwesome6.h>

class CUIViewPort: public CImGuiWindow {
public:
	CUIViewPort() : CImGuiWindow("Viewport") {
		m_Icon = ICON_FA_VIDEO;
		//m_flags = ImGuiWindowFlags_NoMove;
	};
	CUIViewPort(std::string name) : CImGuiWindow(name) {
		m_Icon = ICON_FA_VIDEO; //m_flags = ImGuiWindowFlags_NoMove; 
	};
	CUIViewPort(const CUIViewPort&) = delete;
	CUIViewPort& operator=(const CUIViewPort&) = delete;

	static CUIViewPort& Get() {
		static CUIViewPort UIViewPort("Viewport");
		return UIViewPort;
	}

	void RenderContent() override;

private:
	void DrawToolbar(ImVec2 imageOriginScreen);
	void DrawOverlaysButton(ImVec2 imageOriginScreen, float vpWidth);
	void DrawStatsBadge(ImVec2 imageOriginScreen, ImVec2 vpSize);
	void DrawGizmo(ImVec2 imageOriginScreen, ImVec2 vpSize);
	void DrawViewGizmo(ImVec2 imageOriginScreen, ImVec2 vpSize);

protected:
	bool IconButton(const char* id, const char* icon, ImVec2 size, bool active);
};
