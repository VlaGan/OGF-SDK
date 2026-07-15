//----------------------------------------------------------------------------
//-- CUIMain.h
//-- SDK-style ImGui Docking
//----------------------------------------------------------------------------
#pragma once
#include <Windows.h>
#include <vector>

class CImGuiWindow;

class CUIMain {
public:

	CUIMain();
	~CUIMain();

	//-- singleton
	CUIMain(const CUIMain&) = delete;
	CUIMain& operator=(const CUIMain&) = delete;

	static CUIMain& Get() {
		static CUIMain UIMain;
		return UIMain;
	}

	void Init(HWND wnd);
	void Render();
	void Resize(UINT width, UINT height);

	void DrawDockSpace(); 
	void ApplyModernStyle();

	bool m_bResize{};
	bool m_bFirstFrame{};
	HWND m_hWND{};

private:
	std::vector<CImGuiWindow*> m_Windows;
};
