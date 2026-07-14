//----------------------------------------------------------------------------
//-- CUISceneSettings: general scene settings window for the editor
//----------------------------------------------------------------------------
#include "CUISceneSettings.h"
#include "../Render/CRenderer.h"

void CUISceneSettings::RenderContent()
{
    ImGuiColorEditFlags color_flags =
        ImGuiColorEditFlags_NoSidePreview |
        ImGuiColorEditFlags_NoSmallPreview |
        ImGuiColorEditFlags_PickerHueBar;

    CRenderer& renderer = CRenderer::Get();
    ImGui::Text("Background rt color:");
    ImGui::ColorPicker4("##Color", renderer.m_ClearColor, 0);
    ImGui::Separator();
}
