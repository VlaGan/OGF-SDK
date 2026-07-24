//----------------------------------------------------------------------------
//-- CUIProperties: Scene Selected Item Properties Window
//----------------------------------------------------------------------------
#include "CUIProperties.h"
#include "../Render/CScene.h"
#include "../Render/Model/CModel.h"
#include <fontawesome/IconsFontAwesome6.h>
#include <ctime>
#include <cstdint>
#include <format>


//-- OGF_S_DESC timestamps are unix time (0 = not set/unknown)
static std::string FormatOgfTime(int64_t unixTime)
{
	if (unixTime <= 0)
		return "n/a";
	time_t t = static_cast<time_t>(unixTime);
	std::tm tmv{};
#ifdef _MSC_VER
	localtime_s(&tmv, &t);
#else
	tmv = *std::localtime(&t);
#endif
	char buf[64];
	strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tmv);
	return buf;
}

static const char* OgfShapeTypeName(EOgfBoneShapeType t)
{
	switch (t) {
	case EOgfBoneShapeType::None: return "None";
	case EOgfBoneShapeType::Box: return "Box (OBB)";
	case EOgfBoneShapeType::Sphere: return "Sphere";
	case EOgfBoneShapeType::Cylinder: return "Cylinder";
	default: return "Invalid";
	}
}

//----------------------------------------------------------------------------
//-- Model Info Section (OGF_S_DESC / OGF_S_LODS) - 
//-- only meaningful for natively (.ogf) loaded models
//----------------------------------------------------------------------------
void CUIProperties::RenderModelInfo() {

	CModel* model = CScene::Get().m_SelectedModel;

	if (ImGui::BeginTabItem(ICON_FA_CIRCLE_INFO "  Model Info")) {

		ImGui::Text(ICON_FA_TAG "  Name: %s", model->m_modelName.c_str());

		if (model->m_Description.present) {
			static const char* kCompatNames[] = { "Unknown", "CS/CoP", "SoC", "LegacySDK (v3)" };
			std::string mrefs_format = model->m_OgfSource.compatSignals.sawCopMotionRefs ? "CS/CoP" :
				(model->m_OgfSource.compatSignals.sawSocMotionRefs ? "SoC" : "Unknown");

			ImGui::Text(
				ICON_FA_MICROCHIP "  Detected compat: %s, links [%d], motion refs [%s]",
				kCompatNames[(int)model->m_OgfSource.eModelFormatVer],
				model->m_OgfSource.compatSignals.maxBoneInfluence,
				mrefs_format.c_str()
			);

			ImGui::Text(ICON_FA_FILE_LINES "  Source: %s", model->m_Description.sourceFile.c_str());
			ImGui::Text(ICON_FA_FILE_LINES "  Export tool: %s", model->m_Description.exportTool.c_str());
			ImGui::Text(ICON_FA_CLOCK "  Export time: %s", FormatOgfTime(model->m_Description.exportTime).c_str());
			ImGui::Text(ICON_FA_USER "  Owner: %s", model->m_Description.ownerName.c_str());
			ImGui::Text(ICON_FA_CALENDAR "  Created: %s", FormatOgfTime(model->m_Description.creationTime).c_str());
			ImGui::Text(ICON_FA_FILE_LINES "  Last modified by: %s", model->m_Description.lastModifiedByTool.c_str());
			ImGui::Text(ICON_FA_CLOCK "  Modified time: %s", FormatOgfTime(model->m_Description.modifiedTime).c_str());
		}
		else {
			ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), ICON_FA_CIRCLE_INFO "  No OGF_S_DESC (Assimp model or none present)");
		}

		if (!model->m_LodPath.empty())
			ImGui::Text(ICON_FA_LAYER_GROUP "  LOD path: %s", model->m_LodPath.c_str());

		ImGui::Separator();
		ImGui::EndTabItem();
	}
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Transform Section
//----------------------------------------------------------------------------
void CUIProperties::RenderModelTransform() {

	CScene& scene = CScene::Get();
	CModel* model = scene.m_SelectedModel;

	if (ImGui::BeginTabItem(ICON_FA_UP_DOWN_LEFT_RIGHT "  Transform")) {
		ImGui::InputFloat3("Position", (float*)&model->m_Position.x);
		ImGui::InputFloat3("Rotation (rad)", (float*)&model->m_Rotation.x);
		ImGui::InputFloat3("Scale", (float*)&model->m_Scale.x);
		ImGui::Separator();

		//-- Attaching models
		const std::vector<CModel*>& models = scene.GetModels();

		//-- hack if attach parent was deleted 
		if (model->m_AttachData.valid()) {
			bool reset{ true };
			for (auto& m : models)
				if (m == model->m_AttachData.m_pParent) {
					reset = false;
					break;
				}

			if (reset)
				model->m_AttachData.zero();
		}

		ImGui::SeparatorText("ATTACH DATA");

		ImGui::InputFloat3("Attach position", (float*)&model->m_AttachData.m_attachPos.x);
		ImGui::InputFloat3("Attach rotation", (float*)&model->m_AttachData.m_attachRot.x);
		ImGui::InputFloat3("Attach scale", (float*)&model->m_AttachData.m_attachScale.x);

		if (ImGui::BeginCombo("Attach to##AttachCombo",
			model->m_AttachData.m_pParent ?
			model->m_AttachData.m_pParent->m_modelName.c_str() :
			"No selected attach parent")) {

			for (int i{}; i < models.size(); ++i) {

				//-- ignore selected model and models that attached to selected
				if (model != models[i] && models[i]->m_AttachData.m_pParent != model)
				{
					bool is_selected = model->m_AttachData.m_pParent == models[i];
					if (ImGui::Selectable(models[i]->m_modelName.c_str(), is_selected)) {
						model->m_AttachData.m_pParent = models[i];
					}
					if (is_selected)
						ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}

		if (model->m_AttachData.m_pParent) {
			if (ImGui::BeginCombo("Attach to bone ##AttachComboBone",
				!model->m_AttachData.m_ParentBone.empty() ?
				model->m_AttachData.m_ParentBone.c_str() :
				"No selected attach bone")) {

				for (auto& bone : model->m_AttachData.m_pParent->m_Skeleton.m_vBones) {
					bool is_selected = model->m_AttachData.m_ParentBone == bone.m_sBoneName;
					if (ImGui::Selectable(bone.m_sBoneName.c_str(), is_selected)) {
						model->m_AttachData.m_ParentBone = bone.m_sBoneName;
					}
				}

				ImGui::EndCombo();
			}
		}
		if (ImGui::Button("Detach"))
			model->m_AttachData.zero();

		ImGui::EndTabItem();
	}
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//-- Meshes Data
//----------------------------------------------------------------------------
void CUIProperties::RenderMeshesData() {

	auto& scene = CScene::Get();
	CModel* model = scene.m_SelectedModel;

	SOgfModel& m_ogf = model->m_OgfSource;

	if (ImGui::BeginTabItem(ICON_FA_CUBES "  Meshes")) {

		if (m_ogf.sourcePath.empty()) {
			ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), ICON_FA_CIRCLE_XMARK "  NOT .OGF MODEL SELECTED! (TODO)");
			ImGui::EndTabItem();
			return;
		}

		ImGui::Separator();
		ImGui::Text(ICON_FA_LIST "  Mesh count: %zu", model->m_Meshes.size());
		ImGui::Separator();

		for (size_t i = 0; i < m_ogf.meshes.size(); ++i) {
			CMesh& mesh = model->m_Meshes[i];

			//if (ImGui::TreeNode((void*)(intptr_t)i, "%s", std::format("Mesh [{}]", i).c_str())) {

			ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f),
				ICON_FA_SHAPES "  Mesh [%d] -> Vertices [%d], Faces [%d]",
				i,
				m_ogf.meshes[i].vertices.size(),
				m_ogf.meshes[i].indices.size() / 3
			);

			//-- texture/shader name editing
			char stexture[256], sshader[256];

			strcpy_s(stexture, m_ogf.meshes[i].textureName.c_str());
			strcpy_s(sshader, m_ogf.meshes[i].shaderName.c_str());

			ImGui::Text(ICON_FA_IMAGE "  Texture:");
			ImGui::SameLine();
			if (ImGui::InputText(std::format("###texture_ogf_mesh{}", i).c_str(), stexture, sizeof(stexture)))
				m_ogf.meshes[i].textureName = stexture;

			ImGui::Text(ICON_FA_DRAW_POLYGON "  Shader: ");
			ImGui::SameLine();
			if (ImGui::InputText(std::format("###shader_ogf_mesh{}", i).c_str(), sshader, sizeof(sshader)))
				m_ogf.meshes[i].shaderName = sshader;

			//-- texture preview
			if (model->m_Meshes.size() == m_ogf.meshes.size()) {
				if (ImGui::TreeNode((void*)(intptr_t)i, "Texure Preview")) {
					ImGui::Text("%s [%dx%d]", mesh.m_Texture.GetFileName().c_str(),
						mesh.m_Texture.GetWidth(), mesh.m_Texture.GetHeight());
					float w = ImGui::GetContentRegionAvail().x;
					ImGui::Image(mesh.m_Texture.GetSRV(), ImVec2(w, w));
					ImGui::TreePop();
				}
				if (mesh.IsTextureTransparent())
					ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), ICON_FA_CIRCLE_INFO "  Transparent texture");
			}

			ImGui::Separator();

		}
		ImGui::EndTabItem();
	}

}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//-- Skeleton / Bones Section
//----------------------------------------------------------------------------
void CUIProperties::RenderSkeletonData() {
	
	CModel* model = CScene::Get().m_SelectedModel;

	if (ImGui::BeginTabItem(ICON_FA_BONE "  Skeleton")) {
		if (model->m_Skeleton.m_BoneCount) {
			ImGui::Text(ICON_FA_LIST "  Bone count: %u", model->m_Skeleton.m_BoneCount);
			for (u32 i = 0; i < model->m_Skeleton.m_BoneCount; ++i) {
				CBoneInstance& bone = model->m_Skeleton.m_vBones[i];
				std::string label = "[" + std::to_string(i) + "] " + bone.m_sBoneName;
				if (ImGui::TreeNode((void*)(intptr_t)(1000000 + i), "%s", label.c_str())) {
					ImGui::Text(ICON_FA_TAG "  Name: %s", bone.m_sBoneName.c_str());
					ImGui::Text(ICON_FA_DIAGRAM_PROJECT "  Parent: %s", bone.m_pParent ? bone.m_pParent->m_sBoneName.c_str() : "(root)");
					ImGui::Text(ICON_FA_FILE_LINES "  Material: %s", bone.m_sOgfMaterial.empty() ? "n/a" : bone.m_sOgfMaterial.c_str());
					ImGui::Text(ICON_FA_WEIGHT_HANGING "  Mass: %.3f", bone.m_fOgfMass);
					ImGui::Text(ICON_FA_CIRCLE "  Center of mass: %.3f, %.3f, %.3f",
						bone.m_vOgfCenterOfMass.x, bone.m_vOgfCenterOfMass.y, bone.m_vOgfCenterOfMass.z);

					if (ImGui::TreeNode((void*)(intptr_t)(2000000 + i), ICON_FA_SHAPES "  Shape: %s", OgfShapeTypeName(bone.m_OgfShape.type))) {
						switch (bone.m_OgfShape.type) {
						case EOgfBoneShapeType::Box:
							ImGui::Text("Halfsize: %.3f, %.3f, %.3f", bone.m_OgfShape.box.halfsize.x, bone.m_OgfShape.box.halfsize.y, bone.m_OgfShape.box.halfsize.z);
							ImGui::Text("Translate: %.3f, %.3f, %.3f", bone.m_OgfShape.box.translate.x, bone.m_OgfShape.box.translate.y, bone.m_OgfShape.box.translate.z);
							break;
						case EOgfBoneShapeType::Sphere:
							ImGui::Text("Center: %.3f, %.3f, %.3f", bone.m_OgfShape.sphere.center.x, bone.m_OgfShape.sphere.center.y, bone.m_OgfShape.sphere.center.z);
							ImGui::Text("Radius: %.3f", bone.m_OgfShape.sphere.radius);
							break;
						case EOgfBoneShapeType::Cylinder:
							ImGui::Text("Center: %.3f, %.3f, %.3f", bone.m_OgfShape.cylinder.center.x, bone.m_OgfShape.cylinder.center.y, bone.m_OgfShape.cylinder.center.z);
							ImGui::Text("Direction: %.3f, %.3f, %.3f", bone.m_OgfShape.cylinder.direction.x, bone.m_OgfShape.cylinder.direction.y, bone.m_OgfShape.cylinder.direction.z);
							ImGui::Text("Height: %.3f, Radius: %.3f", bone.m_OgfShape.cylinder.height, bone.m_OgfShape.cylinder.radius);
							break;
						default:
							ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No shape data");
							break;
						}
						ImGui::TreePop();
					}

					if (bone.m_OgfIKData.valid && ImGui::TreeNode((void*)(intptr_t)(3000000 + i), ICON_FA_DIAGRAM_PROJECT "  Joint (IK) data")) {
						ImGui::Text("Joint type: %u", bone.m_OgfIKData.jointType);
						for (int a = 0; a < 3; ++a) {
							const auto& lim = bone.m_OgfIKData.limits[a];
							ImGui::Text("Limit[%d]: %.3f .. %.3f  (spring %.2f, damping %.2f)", a, lim.limit.x, lim.limit.y, lim.springFactor, lim.dampingFactor);
						}
						ImGui::Text("Spring: %.3f  Damping: %.3f", bone.m_OgfIKData.springFactor, bone.m_OgfIKData.dampingFactor);
						ImGui::Text("Break force: %.3f  Break torque: %.3f", bone.m_OgfIKData.breakForce, bone.m_OgfIKData.breakTorque);
						if (bone.m_OgfIKData.frictionValid)
							ImGui::Text("Friction: %.3f", bone.m_OgfIKData.friction);
						else
							ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Friction: n/a (older IK-data format)");
						ImGui::TreePop();
					}

					ImGui::TreePop();
				}
			}
		}
		else {
			ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), ICON_FA_TRIANGLE_EXCLAMATION "  No skeleton loaded");
		}
		ImGui::Separator();

		ImGui::EndTabItem();
	}
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//- Animation Section
//----------------------------------------------------------------------------
void CUIProperties::RenderMotionsData() {

	CScene& scene = CScene::Get();
	CModel* model = scene.m_SelectedModel;

	if (ImGui::BeginTabItem(ICON_FA_PERSON_RUNNING "  Animation")) {
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

						//-- little hack for restart motions on attach 
						// (im thinking about keyframe wnd like in blender)
						for (auto& m : scene.GetModels())
							m->m_CurrentTime = 0.f;

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
			}
			else {
				ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), ICON_FA_TRIANGLE_EXCLAMATION "  No animation playing");
			}
			ImGui::Separator();
		}
		else {
			ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), ICON_FA_TRIANGLE_EXCLAMATION "  No animations loaded");
			ImGui::Separator();
		}
		ImGui::EndTabItem();
	}
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//-- Draw content
//----------------------------------------------------------------------------
void CUIProperties::RenderContent()
{
	auto& scene = CScene::Get();

	if (!scene.m_SelectedModel) {
		ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), ICON_FA_CIRCLE_XMARK "  No item selected!");
		return;
	}

	if (ImGui::BeginTabBar("###tab_model_props")) {
		RenderModelTransform();
		RenderModelInfo();
		RenderMeshesData();
		RenderSkeletonData();
		RenderMotionsData();
		ImGui::EndTabBar();
	}
}
//----------------------------------------------------------------------------
