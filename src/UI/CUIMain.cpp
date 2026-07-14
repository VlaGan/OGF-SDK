//----------------------------------------------------------------------------
//-- CUIMain.cpp
//-- SDK-style ImGui Docking
//----------------------------------------------------------------------------
#include "CUIMain.h"
#include "../Render/CHW.h"
#include <ImGui/imgui.h>
#include <ImGui/imgui_internal.h>
#include <ImGui/imgui_impl_dx11.h>
#include <ImGui/imgui_impl_win32.h>

#include "CUIViewport.h"
#include "CUILog.h"
#include "CUISceneSettings.h"

//-------------------------------------------------------------------------
//-- Ctor / Dtor
//-------------------------------------------------------------------------

CUIMain::CUIMain() {}

CUIMain::~CUIMain()
{
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

//-------------------------------------------------------------------------
//-- Init
//-------------------------------------------------------------------------

void CUIMain::Init(HWND hwnd)
{
    m_hWND = hwnd;

    CHW& hw = CHW::Get();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.IniFilename = "userdata\\imgui.ini"; // disable ini file

    // --- Style  
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 4.0f;
    style.FrameRounding = 3.0f;
    style.ScrollbarRounding = 2.0f;
    style.FrameBorderSize = 1.0f;
    style.GrabRounding = 2.0f;

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.0f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.11f, 0.11f, 0.11f, 1.0f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.16f, 0.16f, 0.16f, 1.0f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.08f, 0.08f, 0.08f, 1.0f);
    colors[ImGuiCol_Header] = ImVec4(0.22f, 0.22f, 0.22f, 1.0f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.28f, 0.28f, 0.28f, 1.0f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.26f, 0.26f, 1.0f);
    colors[ImGuiCol_Button] = ImVec4(0.18f, 0.18f, 0.18f, 1.0f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.24f, 0.24f, 0.24f, 1.0f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.32f, 0.32f, 0.32f, 1.0f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);
    colors[ImGuiCol_Separator] = ImVec4(0.18f, 0.18f, 0.18f, 1.0f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.22f, 0.22f, 0.22f, 1.0f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.26f, 0.26f, 0.26f, 1.0f);
    colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.1f, 0.1f, 0.12f, 1.0f);
    colors[ImGuiCol_DockingPreview] = ImVec4(0.2f, 0.2f, 0.25f, 0.3f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.8f, 0.8f, 0.8f, 1.0f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.25f, 0.5f, 0.7f, 1.0f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.3f, 0.6f, 0.8f, 1.0f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);

    ImGui_ImplWin32_Init(m_hWND);
    ImGui_ImplDX11_Init(hw.m_Device, hw.m_Context);

	//-- Get and add all windows
	m_Windows.push_back(&CUIViewPort::Get());
	m_Windows.push_back(&CUILog::Get());
	m_Windows.push_back(&CUISceneSettings::Get());
}

//-------------------------------------------------------------------------
//-- Resize
//-------------------------------------------------------------------------

void CUIMain::Resize(UINT w, UINT h)
{
    CHW& hw = CHW::Get();
    if (hw.m_Device)
        hw.Resize(w, h);
}

//-------------------------------------------------------------------------
//-- Root DockSpace (EVERY FRAME)
//-------------------------------------------------------------------------
//-- TODO: VlaGan - hmmmm, i need an idea how to make it better using m_Windows
void CUIMain::DrawDockSpace()
{
    ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    if (ImGui::DockBuilderGetNode(dockspace_id) == nullptr)
    {
        ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);
        ImGuiID dock_id_left = 0;
        ImGuiID dock_id_main = dockspace_id;
        ImGui::DockBuilderSplitNode(dock_id_main, ImGuiDir_Right, 0.20f, &dock_id_left, &dock_id_main);
        ImGuiID dock_id_left_top = 0;
        ImGuiID dock_id_left_bottom = 0;
        ImGui::DockBuilderSplitNode(dock_id_left, ImGuiDir_Up, 0.50f, &dock_id_left_top, &dock_id_left_bottom);
        ImGui::DockBuilderDockWindow("Viewport", dock_id_main);
        ImGui::DockBuilderDockWindow("Scene Settings", dock_id_left_top);
        ImGui::DockBuilderDockWindow("Log", dock_id_left_bottom);
        ImGui::DockBuilderFinish(dockspace_id);
    }
    ImGui::DockSpaceOverViewport(dockspace_id, ImGui::GetMainViewport(), 0);

}

//-------------------------------------------------------------------------
//-- Render
//-------------------------------------------------------------------------

void CUIMain::Render()
{
    CHW& hw = CHW::Get();

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    //-- toolbar test
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            ImGui::MenuItem("New");
            ImGui::MenuItem("Open");
            ImGui::MenuItem("Save");
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit")) {
            ImGui::MenuItem("Undo");
            ImGui::MenuItem("Redo");
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View")) {

			for (auto& window : m_Windows)
				ImGui::MenuItem(window->m_Name.c_str(), nullptr, &window->m_Opened);

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    //-- main dock space
    DrawDockSpace();

    //-- windows
    for (auto& window : m_Windows)
        if(window->m_Opened) 
            window->Render();

    ImGui::Render();

    hw.m_Context->OMSetRenderTargets(1, &hw.m_RenderTarget, nullptr);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}
//-------------------------------------------------------------------------
