//----------------------------------------------------------------------------
//-- CUIMain.cpp
//-- SDK-style ImGui Docking
//----------------------------------------------------------------------------

#include "CUIMain.h"

#include "../Render/CHW.h"
#include "../Render/CRenderer.h"

#include <ImGui/imgui.h>
#include <ImGui/imgui_internal.h>
#include <ImGui/imgui_impl_dx11.h>
#include <ImGui/imgui_impl_win32.h>

#include "../Core/CLog.h"
#include "../Core/CCamera.h"
extern CCamera m_Camera;
//-------------------------------------------------------------------------
//-- Ctor / Dtor
//-------------------------------------------------------------------------

CUIMain::CUIMain()
{
}

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
            ImGui::MenuItem("Show Properties");
            ImGui::MenuItem("Show Log");
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    //-- main dock space
    DrawDockSpace();

    //-- windows
    DrawScene();
    DrawProperties();
    DrawLog();

    ImGui::Render();

    hw.m_Context->OMSetRenderTargets(1, &hw.m_RenderTarget, nullptr);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

//-------------------------------------------------------------------------
//-- Viewport
//-------------------------------------------------------------------------
#include <format>
void CUIMain::DrawScene()
{
    CHW& hw = CHW::Get();
    CRenderer& renderer = CRenderer::Get();
    CRenderTarget* rt = renderer.GetMainRT();

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove;
    if (ImGui::Begin("Viewport", nullptr, flags)) {
        ImVec2 size = ImGui::GetContentRegionAvail();
        if (size.x > 0 && size.y > 0)
        {
            if ((UINT)size.x != rt->m_Width || (UINT)size.y != rt->m_Height) {
                rt->Create(hw.m_Device, (UINT)size.x, (UINT)size.y);
				renderer.Resize((UINT)size.x, (UINT)size.y);
            }
        }

        renderer.Render();
        if (rt->m_SRV)
            ImGui::Image(rt->m_SRV, size);

        ImGui::SetCursorPos(ImVec2(10, 28));
		ImGui::Text("VP size: [%.0f, %.0f]", size.x, size.y);
		ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
        ImGui::Text("\nCamera Pos: [%.2f, %.2f, %.2f]", VPUSH3(m_Camera.m_position));

		ImVec2 cursorPos = ImGui::GetCursorPos();
		std::string inputText = std::format("Move: W/S/A/D\nUp/Down: Space/Ctrl\nDirection: Mouse RB+LB");
		ImVec2 textSize = ImGui::CalcTextSize(inputText.c_str());
        ImGui::SetCursorPos(ImVec2(ImGui::GetContentRegionAvail().x - textSize.x, 28));
		ImGui::Text(inputText.c_str());
        ImGui::End();
    }
}

//-------------------------------------------------------------------------
//-- Properties
//-------------------------------------------------------------------------

void CUIMain::DrawProperties()
{
    ImGui::Begin("Scene Settings");

    ImGuiColorEditFlags color_flags =
        ImGuiColorEditFlags_NoSidePreview |
        ImGuiColorEditFlags_NoSmallPreview |
        ImGuiColorEditFlags_PickerHueBar;

    CRenderer& renderer = CRenderer::Get();
    ImGui::Text("Background rt color:");
    ImGui::ColorPicker4("##Color", renderer.m_ClearColor, 0);
    ImGui::Separator();

	ImGui::SliderFloat3("Camera Position", (float*)&m_Camera.m_position, -10.0f, 10.0f);
    ImGui::End();
}

//-------------------------------------------------------------------------
//-- Log
//-------------------------------------------------------------------------
void CUIMain::DrawLog()
{
    if (ImGui::Begin("Log")) {

        CLog& log = CLog::Get();

        if (ImGui::Button("Clear"))
            log.Clear();

        ImGui::SameLine();
        ImGui::Checkbox("Auto-scroll", &log.m_AutoScroll); //-- simple imgui window class needed
        ImGui::SameLine();
        ImGui::Checkbox("Info", &log.m_ShowInfo);
        ImGui::SameLine();
        ImGui::Checkbox("Warning", &log.m_ShowWarning);
        ImGui::SameLine();
        ImGui::Checkbox("Error", &log.m_ShowError);
        ImGui::SameLine();
        ImGui::Checkbox("Debug", &log.m_ShowDebug);

        ImGui::Separator();

        ImGui::BeginChild("LogScrollRegion", ImVec2(0, 0), false,
            ImGuiWindowFlags_HorizontalScrollbar);

        for (const auto& entry : log.GetEntries()) {
            switch (entry.type) {
            case INFO:    if (!log.m_ShowInfo) continue; break;
            case WARNING: if (!log.m_ShowWarning) continue; break;
            case ERR:     if (!log.m_ShowError) continue; break;
            case DEBUG:   if (!log.m_ShowDebug) continue; break;
            }

            ImVec4 color;
            switch (entry.type) {
            case WARNING: color = ImVec4(1.0f, 0.8f, 0.2f, 1.0f); break;
            case ERR:     color = ImVec4(1.0f, 0.3f, 0.3f, 1.0f); break;
            case DEBUG:   color = ImVec4(0.6f, 0.6f, 0.6f, 1.0f); break;
            default:      color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); break;
            }

            ImGui::TextColored(color, "[%s] [%s] %s",
                entry.timestamp.c_str(),
                log.LevelToString(entry.type),
                entry.message.c_str());
        }

        if (log.m_AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            ImGui::SetScrollHereY(1.0f);

        ImGui::EndChild();

        ImGui::End();
    }
}
