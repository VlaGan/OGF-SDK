//----------------------------------------------------------------------------
//-- CUISceneSettings: general scene settings window for the editor
//----------------------------------------------------------------------------
#include "CUISceneSettings.h"
#include "../Render/CRenderer.h"
#include <fontawesome/IconsFontAwesome6.h>
#include "../Render/CScene.h"

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
        CCamera* camera = CScene::Get().Camera();

        std::string camera_mode = camera->m_mode ==
            eCameraProjMode::ePerspective ? "Perspective" : "Orthogonal";

        if (ImGui::Button(ICON_FA_ROTATE_RIGHT "  Camera mode:")) {
            camera->m_mode = camera->m_mode ==
                eCameraProjMode::ePerspective ? eCameraProjMode::Orthographic : eCameraProjMode::ePerspective;
        }
        ImGui::SameLine();
        ImGui::Text(camera_mode.c_str());

        ImGui::InputFloat("zNear", &camera->zNear);
        ImGui::InputFloat("zFar", &camera->zFar);
    }
}
