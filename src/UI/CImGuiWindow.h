//----------------------------------------------------------------------------
//-- CImGUiWindow: Base class for ImGui windows
//----------------------------------------------------------------------------
#pragma once
#include <string>
#include <ImGui/imgui.h>


class CImGuiWindow {
public:
    CImGuiWindow(std::string name);
    virtual ~CImGuiWindow();

    virtual bool RenderBegin();
    virtual void Render();
    virtual void RenderContent();
    virtual void RenderEnd();

    ImGuiWindowFlags_ m_flags{ ImGuiWindowFlags_None };
    std::string m_Name{};
    bool m_Opened{};
    bool m_Collapsed{};
    bool m_bCanHide{ true };
};
