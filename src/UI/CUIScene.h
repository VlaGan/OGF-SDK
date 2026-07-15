//----------------------------------------------------------------------------
//-- CUIScene: Scene UI editor window
//----------------------------------------------------------------------------
#pragma once
#include "CImGuiWindow.h"

class CModel;

class CUIScene: public CImGuiWindow {
public:
	CUIScene() : CImGuiWindow("Scene") {};
	CUIScene(std::string name) : CImGuiWindow(name) {};
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
