//----------------------------------------------------------------------------
//-- COGF_SDK - main window class
//----------------------------------------------------------------------------
#include <ImGui/imgui_internal.h>
#include "OGF_SDK.h"
#include "../Render/CHW.h"
#include "../Render/CRenderer.h"
#include "../UI/CUIMain.h"

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    //-- ImGUI
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg) {

    //-- Changing main win window size.
    //-- Update rts after window resize
    case WM_SIZE: {
        UINT width = LOWORD(lParam);
        UINT height = HIWORD(lParam);
        CUIMain& uiMain = CUIMain::Get();
        uiMain.Resize(width, height);
        /*
        if (::Render && wParam != SIZE_MINIMIZED)
        {
            UINT width = LOWORD(lParam);
            UINT height = HIWORD(lParam);
            ::Render->Resize(width, height);
        }*/
    } break;

        //-- Handle DPI change
    case WM_DPICHANGED:
    {
        UINT newDpiX = HIWORD(wParam);
        UINT newDpiY = LOWORD(wParam);
        float scale = newDpiX / 96.0f;

        //-- I have seen no diff with resizing render/imgui here but leaving it for future tests
        //ResizeRenderer(scale);
        //::Render->Resize(::Render->dwWidth * scale, ::Render->dwHeight * scale);

        RECT* suggestedRect = (RECT*)lParam;
        SetWindowPos(hWnd,
            nullptr,
            suggestedRect->left,
            suggestedRect->top,
            suggestedRect->right - suggestedRect->left,
            suggestedRect->bottom - suggestedRect->top,
            SWP_NOZORDER | SWP_NOACTIVATE);

    }
    break;

    //-- Destroying window
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}


COGF_SDK::COGF_SDK() {
    m_hWND = 0;
    m_dwWidth = 1280;
    m_dwHeight = 720;
}

COGF_SDK::~COGF_SDK() {

}

bool COGF_SDK::Create(HINSTANCE hInstance, int nCmdShow) {
    //-- Setup Windows DPI aware
    //SetProcessDPIAware();
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    //-- create main window
    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    //wc.style = CS_HREDRAW | CS_VREDRAW;
    //wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1)); //-- maiva ico
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"OGFSDKClass";
    wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
    RegisterClass(&wc);

    int wX{}, wY{};
    //CalcCenterWndPlacement(m_uiLoadSizeW, m_uiLoadSizeH, wX, wY);
    m_hWND = CreateWindowEx(0, wc.lpszClassName, L"OGF SDK",
        WS_OVERLAPPEDWINDOW ,
        wX, wY, m_dwWidth, m_dwHeight, nullptr, nullptr, hInstance, nullptr);

    //SetLayeredWindowAttributes(m_hWnd, RGB(0, 0, 0), 0, LWA_COLORKEY);
    ShowWindow(m_hWND, nCmdShow);

    CHW& dhw = CHW::Get();
    dhw.Init(m_hWND);

    CRenderer& render = CRenderer::Get();
    render.Init(800, 600);

    CUIMain& uiMain = CUIMain::Get();
    uiMain.Init(m_hWND);

    return true;

}

bool COGF_SDK::Run() {
    MSG msg = {};
    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else 
            Render();
    }
    return 0;
}


void COGF_SDK::Render() {
    CHW& hw = CHW::Get();

    CUIMain& uiMain = CUIMain::Get();
    uiMain.Render();

    hw.m_SwapChain->Present(0, 0);
}
