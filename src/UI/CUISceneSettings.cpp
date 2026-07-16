//----------------------------------------------------------------------------
//-- CUISceneSettings: general scene settings window for the editor
//----------------------------------------------------------------------------
#include "CUISceneSettings.h"
#include "../Render/CRenderer.h"
#include <fontawesome/IconsFontAwesome6.h>

#include "../Core/CCamera.h"

extern CCamera m_Camera;

void CUISceneSettings::RenderContent()
{
    if (ImGui::CollapsingHeader(ICON_FA_GEAR "  Genearal", ImGuiTreeNodeFlags_DefaultOpen)) {

        ImGuiColorEditFlags color_flags =
            ImGuiColorEditFlags_NoSidePreview |
            ImGuiColorEditFlags_NoSmallPreview |
            ImGuiColorEditFlags_PickerHueBar;

        CRenderer& renderer = CRenderer::Get();
        ImGui::Text(ICON_FA_PALETTE "  Background rt color:");
        ImGui::ColorPicker4("##Color", renderer.m_ClearColor, 0);
        ImGui::Separator();
    }

    if (ImGui::CollapsingHeader(ICON_FA_CAMERA "  Camera", ImGuiTreeNodeFlags_None)) {

        std::string camera_mode = m_Camera.m_mode == 
            eCameraProjMode::ePerspective ? "Perspective" : "Orthogonal";

        if (ImGui::Button(ICON_FA_ROTATE_RIGHT "  Camera mode:")) {
            m_Camera.m_mode = m_Camera.m_mode ==
                eCameraProjMode::ePerspective ? eCameraProjMode::Orthographic : eCameraProjMode::ePerspective;
        }
        ImGui::SameLine();
        ImGui::Text(camera_mode.c_str());

        ImGui::InputFloat("zNear", &m_Camera.zNear);
        ImGui::InputFloat("zFar", &m_Camera.zFar);
    }
}
