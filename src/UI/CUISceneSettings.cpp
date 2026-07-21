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

        ImGui::SliderFloat("Mouse sense", &camera->m_mouseSensitivity, 0.001f, 0.01f);
        ImGui::SliderFloat("FOV", &camera->m_fov, 0.01f, 1.f);

        ImGui::InputFloat("zNear", &camera->zNear);
        ImGui::InputFloat("zFar", &camera->zFar);
    }
}
