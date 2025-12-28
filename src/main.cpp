//----------------------------------------------------------------------------
//-- OGF SDK
//-- VlaGan: Entry Point
//----------------------------------------------------------------------------
#include "Core/OGF_SDK.h"
#include <Windows.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    COGF_SDK app;
    if (!app.Create(hInstance, nCmdShow))
        return -1;

    int result = app.Run();
    app.OnDestroy();

    return result;
}
