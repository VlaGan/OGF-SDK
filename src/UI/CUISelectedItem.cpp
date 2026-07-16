//----------------------------------------------------------------------------
//-- CUISelectedItemProp: Scene Selected Item Properties Window
//----------------------------------------------------------------------------
#include "CUISelectedItem.h"
#include "../Render/CScene.h"
#include "../Render/Model/CModel.h"
#include <fontawesome/IconsFontAwesome6.h>

void CUISelectedItemProp::RenderContent()
{
	auto& scene = CScene::Get();

	if (!scene.m_SelectedModel) {
		ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), ICON_FA_CIRCLE_XMARK "  No item selected!");
		return;
	}

	CModel* model = scene.m_SelectedModel;

	// Transform Section
	if (ImGui::CollapsingHeader(ICON_FA_UP_DOWN_LEFT_RIGHT "  Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::InputFloat3("Position", (float*)&model->m_Position.x);
		ImGui::InputFloat3("Rotation (rad)", (float*)&model->m_Rotation.x);
		ImGui::InputFloat3("Scale", (float*)&model->m_Scale.x);
		ImGui::Separator();
	}

	// Animation Section
	if (ImGui::CollapsingHeader(ICON_FA_PERSON_RUNNING "  Animation", ImGuiTreeNodeFlags_DefaultOpen)) {
		if (!model->m_vMotions.empty()) {
			// Motion Selection Dropdown
			static int current_motion_index = 0;

			if (ImGui::BeginCombo("active motion##MotionCombo", 
				model->m_pCurrentMotion ? model->m_pCurrentMotion->GetName().c_str() : "Select Motion")) {
				for (int i = 0; i < (int)model->m_vMotions.size(); i++) {
					bool is_selected = (model->m_pCurrentMotion == &model->m_vMotions[i]);
					if (ImGui::Selectable(model->m_vMotions[i].GetName().c_str(), is_selected)) {
						model->SetMotion(&model->m_vMotions[i]);
						current_motion_index = i;
					}
					if (is_selected) {
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}

			ImGui::Text(ICON_FA_FILM "  Motions: %zu available", model->m_vMotions.size());

			if (model->m_pCurrentMotion) {
				ImGui::Text(ICON_FA_STOPWATCH "  Duration: %.2f sec", model->m_pCurrentMotion->Duration());
				ImGui::Text(ICON_FA_CLOCK "  Current Time: %.2f sec", model->m_CurrentTime);
				ImGui::Text(ICON_FA_GAUGE_HIGH "  Ticks Per Second: %.2f", model->m_pCurrentMotion->TPS());
			} else {
				ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), ICON_FA_TRIANGLE_EXCLAMATION "  No animation playing");
			}
			ImGui::Separator();
		} else {
			ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), ICON_FA_TRIANGLE_EXCLAMATION "  No animations loaded");
			ImGui::Separator();
		}
	}
}
