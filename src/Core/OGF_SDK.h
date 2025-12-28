//----------------------------------------------------------------------------
//-- COGF_SDK - main window class
//----------------------------------------------------------------------------
#pragma once
#include <Windows.h>

class COGF_SDK {
public:
	COGF_SDK();
	~COGF_SDK();
	void OnDestroy() {};


	bool Create(HINSTANCE hInstance, int nCmdShow);
	void Render();
	bool Run();

private: 
	HWND m_hWND;

	UINT m_dwWidth{};
	UINT m_dwHeight{};
};