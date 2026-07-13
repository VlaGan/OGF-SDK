//----------------------------------------------------------------------------
//-- CHW - DX11 hardware
//----------------------------------------------------------------------------
#include "CHW.h"
#include "../_defines.h"

CHW::~CHW() {
    RELEASE(m_Device);
    RELEASE(m_Context);
    RELEASE(m_SwapChain);
    RELEASE(m_RenderTarget);
} 

bool CHW::Init(HWND wnd) {
    m_hWNDref = wnd;
    /*dwWidth = width;
    dwHeight = height;
    h_Wnd = hWnd;*/

    //-- Better GPU selection
    IDXGIFactory* dxgiFactory = nullptr;
    CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&dxgiFactory);

    IDXGIAdapter* selectedAdapter = nullptr;
    IDXGIAdapter* adapter = nullptr;

    for (UINT i = 0; dxgiFactory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i) {
        DXGI_ADAPTER_DESC desc;
        adapter->GetDesc(&desc);

        if (desc.DedicatedVideoMemory > 1024 * 1024 * 1024) {
            // VendorId:
            // NVIDIA: 0x10DE, AMD: 0x1002, Intel: 0x8086
            if (desc.VendorId != 0x8086) { // Skip Intel integrated graphics
                selectedAdapter = adapter;
                break;
            }
        }
        adapter->Release();
    }

    // if only integrated GPU found, use it
    if (selectedAdapter == nullptr)
        dxgiFactory->EnumAdapters(0, &selectedAdapter);

    DXGI_SWAP_CHAIN_DESC scd = {};
    scd.BufferCount = 1;
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.OutputWindow = m_hWNDref;
    scd.SampleDesc.Count = 1;
    scd.Windowed = TRUE;
    scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        selectedAdapter,
        selectedAdapter ? D3D_DRIVER_TYPE_UNKNOWN : D3D_DRIVER_TYPE_HARDWARE,
        nullptr, 0, nullptr, 0,
        D3D11_SDK_VERSION, &scd, &m_SwapChain, &m_Device, nullptr, &m_Context);

    if (selectedAdapter) selectedAdapter->Release();
    if (dxgiFactory) dxgiFactory->Release();

    //-- Render target
    ID3D11Texture2D* backBuffer = nullptr;
    m_SwapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    m_Device->CreateRenderTargetView(backBuffer, nullptr, &m_RenderTarget);
    backBuffer->Release();

    return SUCCEEDED(hr);
}

bool CHW::Resize(UINT w, UINT h) {
    RELEASE(m_RenderTarget);

    HRESULT hr = m_SwapChain->ResizeBuffers(0, w, h, DXGI_FORMAT_UNKNOWN, 0);
    if (FAILED(hr)) return false;

    ID3D11Texture2D* backBuffer = nullptr;
    m_SwapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    m_Device->CreateRenderTargetView(backBuffer, nullptr, &m_RenderTarget);
    backBuffer->Release();

    return true;
}
