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
#include "CUIScene.h"
#include "CUISelectedItem.h"
#include <fontawesome/IconsFontAwesome6.h>
#include <ImGuizmo.h>

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
//-- Apply Modern Style
//-------------------------------------------------------------------------
void CUIMain::ApplyModernStyle()
{
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();

    // --- Geometry ---------------------------------------------------
    style.WindowRounding = 8.0f;
    style.ChildRounding = 6.0f;
    style.FrameRounding = 6.0f;
    style.PopupRounding = 6.0f;
    style.ScrollbarRounding = 8.0f;
    style.GrabRounding = 6.0f;
    style.TabRounding = 6.0f;

    style.WindowBorderSize = 0.0f;
    style.ChildBorderSize = 1.0f;
    style.PopupBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;

    style.WindowPadding = ImVec2(12, 12);
    style.FramePadding = ImVec2(10, 5);
    style.CellPadding = ImVec2(8, 4);
    style.ItemSpacing = ImVec2(10, 8);
    style.ItemInnerSpacing = ImVec2(6, 6);
    style.IndentSpacing = 18.0f;
    style.ScrollbarSize = 14.0f;
    style.GrabMinSize = 10.0f;

    style.WindowTitleAlign = ImVec2(0.02f, 0.5f);
    style.SeparatorTextBorderSize = 1.0f;
    style.DockingSeparatorSize = 2.0f;

    // --- Palette ------------------------------------------------------
    const ImVec4 bg0 = ImVec4(0.071f, 0.078f, 0.090f, 1.00f); // deepest bg
    const ImVec4 bg1 = ImVec4(0.098f, 0.106f, 0.122f, 1.00f); // panel bg
    const ImVec4 bg2 = ImVec4(0.129f, 0.141f, 0.161f, 1.00f); // widget bg
    const ImVec4 bg3 = ImVec4(0.161f, 0.176f, 0.200f, 1.00f); // hovered widget
    const ImVec4 bg4 = ImVec4(0.200f, 0.216f, 0.243f, 1.00f); // active widget
    const ImVec4 border = ImVec4(0.220f, 0.235f, 0.259f, 0.60f);
    const ImVec4 text = ImVec4(0.902f, 0.914f, 0.925f, 1.00f);
    const ImVec4 textDim = ImVec4(0.557f, 0.588f, 0.620f, 1.00f);
    const ImVec4 accent = ImVec4(0.259f, 0.784f, 0.714f, 1.00f); // teal
    const ImVec4 accentHi = ImVec4(0.361f, 0.867f, 0.796f, 1.00f);
    const ImVec4 accentLo = ImVec4(0.259f, 0.784f, 0.714f, 0.35f);

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_Text] = text;
    colors[ImGuiCol_TextDisabled] = textDim;
    colors[ImGuiCol_WindowBg] = bg1;
    colors[ImGuiCol_ChildBg] = bg1;
    colors[ImGuiCol_PopupBg] = bg1;
    colors[ImGuiCol_Border] = border;
    colors[ImGuiCol_BorderShadow] = ImVec4(0, 0, 0, 0);

    colors[ImGuiCol_FrameBg] = bg2;
    colors[ImGuiCol_FrameBgHovered] = bg3;
    colors[ImGuiCol_FrameBgActive] = bg4;

    colors[ImGuiCol_TitleBg] = bg0;
    colors[ImGuiCol_TitleBgActive] = bg0;
    colors[ImGuiCol_TitleBgCollapsed] = bg0;
    colors[ImGuiCol_MenuBarBg] = bg0;

    colors[ImGuiCol_ScrollbarBg] = ImVec4(0, 0, 0, 0);
    colors[ImGuiCol_ScrollbarGrab] = bg3;
    colors[ImGuiCol_ScrollbarGrabHovered] = bg4;
    colors[ImGuiCol_ScrollbarGrabActive] = accent;

    colors[ImGuiCol_CheckMark] = accentHi;
    colors[ImGuiCol_SliderGrab] = accent;
    colors[ImGuiCol_SliderGrabActive] = accentHi;

    colors[ImGuiCol_Button] = bg3;
    colors[ImGuiCol_ButtonHovered] = bg4;
    colors[ImGuiCol_ButtonActive] = accentLo;

    colors[ImGuiCol_Header] = bg3;
    colors[ImGuiCol_HeaderHovered] = bg4;
    colors[ImGuiCol_HeaderActive] = accentLo;

    colors[ImGuiCol_Separator] = border;
    colors[ImGuiCol_SeparatorHovered] = accent;
    colors[ImGuiCol_SeparatorActive] = accentHi;

    colors[ImGuiCol_ResizeGrip] = ImVec4(0, 0, 0, 0);
    colors[ImGuiCol_ResizeGripHovered] = accentLo;
    colors[ImGuiCol_ResizeGripActive] = accent;

    colors[ImGuiCol_Tab] = bg1;
    colors[ImGuiCol_TabHovered] = bg3;
    colors[ImGuiCol_TabActive] = bg2;
    colors[ImGuiCol_TabUnfocused] = bg1;
    colors[ImGuiCol_TabUnfocusedActive] = bg2;

    colors[ImGuiCol_DockingPreview] = accentLo;
    colors[ImGuiCol_DockingEmptyBg] = bg0;

    colors[ImGuiCol_PlotLines] = accent;
    colors[ImGuiCol_PlotLinesHovered] = accentHi;
    colors[ImGuiCol_PlotHistogram] = accent;
    colors[ImGuiCol_PlotHistogramHovered] = accentHi;

    colors[ImGuiCol_TableHeaderBg] = bg2;
    colors[ImGuiCol_TableBorderStrong] = border;
    colors[ImGuiCol_TableBorderLight] = ImVec4(border.x, border.y, border.z, 0.30f);
    colors[ImGuiCol_TableRowBg] = ImVec4(0, 0, 0, 0);
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(1, 1, 1, 0.02f);

    colors[ImGuiCol_TextSelectedBg] = accentLo;
    colors[ImGuiCol_DragDropTarget] = accentHi;
    colors[ImGuiCol_NavHighlight] = accent;
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
    io.ConfigWindowsMoveFromTitleBarOnly = true;
    io.IniFilename = "userdata\\imgui.ini"; // disable ini file

    // --- Font ----------------------------------------------------------
    // Base UI font: Segoe UI (falls back to ImGui's default if missing).
    ImFontConfig fontCfg;
    fontCfg.OversampleH = 3;
    fontCfg.OversampleV = 3;
    fontCfg.RasterizerMultiply = 1.05f;
    io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 17.0f, &fontCfg);

    // Merge FontAwesome 6 (solid) glyphs into the same font so icons can be
    // dropped straight into any text/label string, like ICON_FA_FLOPPY_DISK " Save".
    static const ImWchar iconRanges[] = { ICON_MIN_FA, ICON_MAX_16_FA, 0 };
    ImFontConfig iconCfg;
    iconCfg.MergeMode = true;
    iconCfg.PixelSnapH = true;
    iconCfg.GlyphMinAdvanceX = 17.0f; // keep icons monospaced-ish next to text
    iconCfg.GlyphOffset = ImVec2(0.0f, 1.0f);
    io.Fonts->AddFontFromFileTTF("appdata\\fonts\\fa-solid-900.ttf", 15.0f, &iconCfg, iconRanges);

    io.Fonts->Build();

    // --- Style -----------------------------------------------------------
    ApplyModernStyle();

    ImGui_ImplWin32_Init(m_hWND);
    ImGui_ImplDX11_Init(hw.m_Device, hw.m_Context);

    //-- Get and add all windows
    m_Windows.push_back(&CUIViewPort::Get());
    m_Windows.push_back(&CUILog::Get());
    m_Windows.push_back(&CUISceneSettings::Get());
    m_Windows.push_back(&CUIScene::Get());
    m_Windows.push_back(&CUISelectedItemProp::Get());
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
        ImGui::DockBuilderDockWindow("Scene", dock_id_left_top);
        ImGui::DockBuilderDockWindow("Properties", dock_id_left_top);
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
    ImGuizmo::BeginFrame(); // must run once per frame, before any Manipulate() calls

    //-- main menu bar
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12.0f, 8.0f));
    if (ImGui::BeginMainMenuBar()) {

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.361f, 0.867f, 0.796f, 1.0f));
        ImGui::TextUnformatted(ICON_FA_CUBES "  OGF-SDK");
        ImGui::PopStyleColor();
        ImGui::Separator();

        if (ImGui::BeginMenu(ICON_FA_FILE "  File")) {
            ImGui::MenuItem(ICON_FA_FILE "  New");
            ImGui::MenuItem(ICON_FA_FOLDER_OPEN "  Open");
            ImGui::MenuItem(ICON_FA_FLOPPY_DISK "  Save");
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu(ICON_FA_PEN "  Edit")) {
            ImGui::MenuItem(ICON_FA_ROTATE_LEFT "  Undo");
            ImGui::MenuItem(ICON_FA_ROTATE_RIGHT "  Redo");
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu(ICON_FA_EYE "  View")) {

            for (auto& window : m_Windows)
                ImGui::MenuItem(window->TitleWithIcon().c_str(), nullptr, &window->m_Opened);

            ImGui::EndMenu();
        }

        //-- right-aligned FPS readout
        char fpsBuf[32];
        snprintf(fpsBuf, sizeof(fpsBuf), ICON_FA_GAUGE_HIGH "  %.0f FPS", ImGui::GetIO().Framerate);
        float fpsWidth = ImGui::CalcTextSize(fpsBuf).x;
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - fpsWidth - 16.0f);
        ImGui::TextDisabled("%s", fpsBuf);

        ImGui::EndMainMenuBar();
    }
    ImGui::PopStyleVar();

    //-- main dock space
    DrawDockSpace();

    //-- windows
    for (auto& window : m_Windows)
        if (window->m_Opened)
            window->Render();

    ImGui::Render();

    hw.m_Context->OMSetRenderTargets(1, &hw.m_RenderTarget, nullptr);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}
//-------------------------------------------------------------------------
