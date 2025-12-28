//----------------------------------------------------------------------------
//-- CHW - DX11 hardware
//----------------------------------------------------------------------------
#pragma once
#include <d3d11.h>

class CHW {
public:
	CHW() = default;
	~CHW();

	//-- singleton
	CHW(const CHW&) = delete;
	CHW& operator=(const CHW&) = delete;

	static CHW& Get() {
		static CHW D3D11HW;
		return D3D11HW;
	}

	bool Init(HWND wnd);
	bool Resize(UINT w, UINT h);

	IDXGISwapChain* m_SwapChain{};
	ID3D11Device* m_Device{};
	ID3D11DeviceContext* m_Context{};
	ID3D11RenderTargetView* m_RenderTarget{};

private:
	HWND m_hWNDref;
};