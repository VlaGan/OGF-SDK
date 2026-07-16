//----------------------------------------------------------------------------
//-- CUIScene: Scene UI editor window
//----------------------------------------------------------------------------
#pragma once
#include "CImGuiWindow.h"
#include <fontawesome/IconsFontAwesome6.h>

class CModel;

class CUIScene: public CImGuiWindow {
public:
	CUIScene() : CImGuiWindow("Scene") { m_Icon = ICON_FA_FOLDER_TREE; };
	CUIScene(std::string name) : CImGuiWindow(name) { m_Icon = ICON_FA_FOLDER_TREE; };
	CUIScene(const CUIScene&) = delete;
	CUIScene& operator=(const CUIScene&) = delete;

	static CUIScene& Get() {
		static CUIScene UIScene("Scene");
		return UIScene;
	}

	void RenderContent() override;

private:
	void DrawModelNode(CModel* model);
};
