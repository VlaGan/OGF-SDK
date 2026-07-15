//----------------------------------------------------------------------------
//-- CUIViewPort: Viewport window for ImGui Docking
//----------------------------------------------------------------------------
#include "CUIScene.h"
#include "../_defines.h"
#include "../Render/CScene.h"
#include "../Render/Model/CModel.h"
#include <format>


//-------------------------------------------------------------------------
//-- RenderContent
//-------------------------------------------------------------------------
void CUIScene::RenderContent()
{
    CScene& scene = CScene::Get();

    ImGui::Checkbox("Grid", &scene.m_bDrawGrid);
    ImGui::SameLine();
    ImGui::Checkbox("Wireframe", &scene.m_bWireframe);
    ImGui::SameLine();
    ImGui::Checkbox("DrawSkeleton", &scene.m_bDrawSkeleton);
    ImGui::Separator();

    //----------------------------------------
    // Search
    //----------------------------------------
    static char filter[128]{};

    ImGui::SetNextItemWidth(-1);
    ImGui::InputTextWithHint("##scene_filter", "Search...", filter, sizeof(filter));
    ImGui::Separator();

	CModel* g_SelectedModel = scene.m_SelectedModel;
    if (g_SelectedModel) {
		ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Selected Model: %s", g_SelectedModel->m_modelName.c_str());
    }else
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Selected Model: None");
    ImGui::Separator();

    //----------------------------------------
    // Scene
    //----------------------------------------

    if (ImGui::TreeNodeEx("Scene", ImGuiTreeNodeFlags_DefaultOpen))
    {
        auto& models = scene.GetModels();
        for (size_t i = 0; i < models.size(); i++)
        {
            CModel* model = models[i];
            if (!filter[0] || model->m_modelName.find(filter) != std::string::npos)
                DrawModelNode(model);
        }
        ImGui::TreePop();
    }
}

void CUIScene::DrawModelNode(CModel* model)
{
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow |ImGuiTreeNodeFlags_SpanAvailWidth;

    CScene& scene = CScene::Get();

    if (scene.m_SelectedModel == model)
        flags |= ImGuiTreeNodeFlags_Selected;

    bool opened = ImGui::TreeNodeEx(model, flags, "%s", model->m_modelName.c_str());

    if (ImGui::IsItemClicked())
        scene.m_SelectedModel = model;

    //-------------------------------------
    // Context menu
    //-------------------------------------

    if (ImGui::BeginPopupContextItem())
    {
        if (ImGui::MenuItem("Rename")) {}
        if (ImGui::MenuItem("Delete")) {
            scene.DeleteModel(model);
            return;
        }

        ImGui::Separator();
        ImGui::TextDisabled("Meshes: %d", (int)model->m_Meshes.size());
        ImGui::EndPopup();
    }

    if (!opened)
        return;

    //-------------------------------------
    // Meshes
    //-------------------------------------
    if (ImGui::TreeNodeEx("Meshes", ImGuiTreeNodeFlags_DefaultOpen))
    {
        for (size_t i = 0; i < model->m_Meshes.size(); i++)
        {
            auto& mesh = model->m_Meshes[i];
            ImGui::Bullet();
            ImGui::SameLine();
            ImGui::Selectable(std::format("Mesh {}  [{} verts]", i, mesh.m_VertexCount).c_str());
        }
        ImGui::TreePop();
    }

    //-------------------------------------
    // Skeleton
    //-------------------------------------
    if (model->m_Skeleton.m_BoneCount)
    {
        if (ImGui::TreeNode("Skeleton"))
        {
            for (auto& bone : model->m_Skeleton.m_vBones)
            {
                ImGui::Bullet();
                ImGui::SameLine();
                ImGui::Selectable(bone.m_sBoneName.c_str());
            }
            ImGui::TreePop();
        }
    }

    //-------------------------------------
    // Animations
    //-------------------------------------
    if (!model->m_vMotions.empty())
    {
        if (ImGui::TreeNode("Animations"))
        {
            for (auto& anim : model->m_vMotions)
            {
                ImGui::Bullet();
                ImGui::SameLine();
                ImGui::Selectable(anim.GetName().c_str());
            }
            ImGui::TreePop();
        }
    }

    ImGui::TreePop();
}
