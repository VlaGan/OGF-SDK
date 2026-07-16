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
    std::string m_Icon{};
    bool m_Opened{};
    bool m_Collapsed{};
    bool m_bCanHide{ true };

    //-- Visible label with icon, while keeping the ImGui ID (used by
    //-- docking / window lookup) equal to the plain m_Name via "###".
    std::string TitleWithIcon() const {
        return (m_Icon.empty() ? m_Name : m_Icon + "  " + m_Name) + "###" + m_Name;
    }
};
