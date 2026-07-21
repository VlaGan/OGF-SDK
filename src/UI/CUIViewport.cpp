//----------------------------------------------------------------------------
//-- CUIViewPort: Viewport window for ImGui Docking
//----------------------------------------------------------------------------
#include "CUIViewport.h"
#include <format>
#include <algorithm>
#include "../_defines.h"
#include "../Render/CHW.h"
#include "../Render/CRenderer.h"
#include "../Render/CScene.h"
#include "../Render/Model/CModel.h"
#include "../Core/CCamera.h"
#include <fontawesome/IconsFontAwesome6.h>
#include <ImGui/imgui_internal.h> 
#include <ImGuizmo.h>


//-- shared visual constants for the floating viewport widgets
constexpr float kToolBtn = 28.0f;
constexpr float kMargin = 10.0f;
const ImVec4 kCardBg = ImVec4(0.071f, 0.078f, 0.090f, 0.72f);
const ImVec4 kAccent = ImVec4(0.259f, 0.784f, 0.714f, 1.00f);
const ImVec4 kAccentLo = ImVec4(0.259f, 0.784f, 0.714f, 0.30f);

//---------------------------------------------------------------------
//-- IconButton
//-- Advanced drawing font awesome font image button
//---------------------------------------------------------------------
bool CUIViewPort::IconButton(const char* id, const char* icon, ImVec2 size, bool active)
{
	ImVec2 pos = ImGui::GetCursorScreenPos();

	ImGui::PushID(id);
	bool pressed = ImGui::InvisibleButton("##btn", size);
	bool hovered = ImGui::IsItemHovered();
	bool held = ImGui::IsItemActive();
	ImGui::PopID();

	ImDrawList* dl = ImGui::GetWindowDrawList();

	ImU32 bg;
	if (active)       bg = ImGui::GetColorU32(kAccentLo);
	else if (held)    bg = ImGui::GetColorU32(ImGuiCol_ButtonActive);
	else if (hovered) bg = ImGui::GetColorU32(ImGuiCol_ButtonHovered);
	else              bg = ImGui::GetColorU32(ImGuiCol_Button);

	dl->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), bg, ImGui::GetStyle().FrameRounding);

	ImFont* font = ImGui::GetFont();
	ImFontBaked* baked = ImGui::GetFontBaked(); // baked glyph data for the currently bound font+size
	unsigned int codepoint = 0;
	ImTextCharFromUtf8(&codepoint, icon, nullptr);
	const ImFontGlyph* glyph = baked->FindGlyph((ImWchar)codepoint);
	ImU32 col = active ? ImGui::GetColorU32(kAccent) : ImGui::GetColorU32(ImGuiCol_Text);

	if (glyph) {
		ImVec2 center(pos.x + size.x * 0.5f, pos.y + size.y * 0.5f);
		float glyphW = glyph->X1 - glyph->X0;
		float glyphH = glyph->Y1 - glyph->Y0;
		ImVec2 drawPos(center.x - glyphW * 0.5f - glyph->X0, center.y - glyphH * 0.5f - glyph->Y0);
		dl->AddText(font, ImGui::GetFontSize(), drawPos, col, icon);
	}
	else {
		ImVec2 textSize = ImGui::CalcTextSize(icon);
		dl->AddText(ImVec2(pos.x + (size.x - textSize.x) * 0.5f, pos.y + (size.y - textSize.y) * 0.5f), col, icon);
	}

	return pressed;
}


//-------------------------------------------------------------------------
//-- DrawToolbar Select
//-- Select / Move / Rotate / Scale — drives both the ImGuizmo operation and the
//-- highlight below.
//-------------------------------------------------------------------------
void CUIViewPort::DrawToolbar(ImVec2 imageOriginScreen)
{
	CScene& scene = CScene::Get();

	struct ToolBtn { eGizmoMode mode; const char* icon; const char* tip; };
	static const ToolBtn tools[] = {
		{ eGizmoMode::eSelect,    ICON_FA_ARROW_POINTER, "Select (Q)" },
		{ eGizmoMode::eTranslate, ICON_FA_UP_DOWN_LEFT_RIGHT, "Move (W)" },
		{ eGizmoMode::eRotate,    ICON_FA_ROTATE, "Rotate (E)" },
		{ eGizmoMode::eScale,     ICON_FA_MAXIMIZE, "Scale (R)" },
	};
	constexpr int   kToolCount = (int)(sizeof(tools) / sizeof(tools[0]));
	constexpr float kPad = 6.0f;  // child's own inner padding
	constexpr float kGap = 6.0f;  // spacing between stacked buttons

	ImVec2 childSize(
		kToolBtn + kPad * 2.0f,
		kPad * 2.0f + kToolBtn * kToolCount + kGap * (kToolCount - 1));

	ImVec2 pos(imageOriginScreen.x + kMargin, imageOriginScreen.y + kMargin);
	ImGui::SetCursorScreenPos(pos);
	ImGui::PushStyleColor(ImGuiCol_ChildBg, kCardBg);
	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(kPad, kPad));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(kGap, kGap));
	ImGui::BeginChild("##vp_toolbar", childSize, true,
		ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

	for (auto& tool : tools)
	{
		bool active = scene.m_GizmoMode == tool.mode;

		if (IconButton(tool.tip, tool.icon, ImVec2(kToolBtn, kToolBtn), active))
			scene.m_GizmoMode = tool.mode;

		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("%s", tool.tip);
	}

	ImGui::EndChild();
	ImGui::PopStyleVar(3);
	ImGui::PopStyleColor();

	//-- Display gizmo mode | selected model name
	pos.x = pos.x + childSize.x + kMargin;
	pos.y += 10.f;
	ImGui::SetCursorScreenPos(pos);
	std::string desc_text = "NO SELECTED MODEL";
	ImVec4 desc_color(1.f, 1.f, 0.f, 1.f);

	if (scene.m_SelectedModel) {

		switch (scene.m_GizmoMode) {
			case eGizmoMode::eSelect: desc_text = "SELECT | "; break;
			case eGizmoMode::eTranslate: desc_text = "TRANSLATE | "; break;
			case eGizmoMode::eRotate: desc_text = "ROTATE | "; break;
			case eGizmoMode::eScale: desc_text = "SCALE | "; break;
			default: desc_text = "NONE | "; break;
		}

		desc_text += scene.m_SelectedModel->m_modelName;
	}

	ImGui::TextColored(desc_color, desc_text.c_str());
}

//-------------------------------------------------------------------------
//-- DrawOverlaysButton
//-- Blender-style "Overlays" dropdown, top-right corner of the image, plus
//-- a World/Local space toggle for the gizmo stacked right under it.
//-------------------------------------------------------------------------
void CUIViewPort::DrawOverlaysButton(ImVec2 imageOriginScreen, float vpWidth)
{
	CScene& scene = CScene::Get();
	
	float btnSize = 36.f;
	float btnWidth = 104.0f;
	ImVec2 btnPos(imageOriginScreen.x + vpWidth - btnWidth - kMargin, imageOriginScreen.y + kMargin);
	ImGui::SetCursorScreenPos(btnPos);

	ImGui::PushStyleColor(ImGuiCol_Button, kCardBg);
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.161f, 0.176f, 0.200f, 0.85f));
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);

	if (ImGui::Button(ICON_FA_LAYER_GROUP "  Overlays " ICON_FA_CARET_DOWN, ImVec2(btnWidth, btnSize)))
		ImGui::OpenPopup("##vp_overlays_popup");

	ImVec2 btnMin = ImGui::GetItemRectMin();
	ImVec2 btnMax = ImGui::GetItemRectMax();

	ImGui::PopStyleVar();
	ImGui::PopStyleColor(2);

	ImGui::SetNextWindowPos(ImVec2(btnMin.x, btnMax.y + 4.0f));
	if (ImGui::BeginPopup("##vp_overlays_popup"))
	{
		ImGui::TextDisabled("VIEWPORT OVERLAYS");
		ImGui::Separator();
		ImGui::Checkbox(ICON_FA_BORDER_ALL "  Grid", &scene.m_bDrawGrid);
		ImGui::Checkbox(ICON_FA_VECTOR_SQUARE "  Wireframe", &scene.m_bWireframe);
		ImGui::Checkbox(ICON_FA_BONE "  Skeleton", &scene.m_bDrawSkeleton);
		ImGui::EndPopup();
	}

	//-- World/Local gizmo-space toggle, stacked directly under Overlays
	ImGui::SetCursorScreenPos(ImVec2(btnMax.x - btnSize, btnMax.y + 6.0f));
	const char* spaceIcon = scene.m_bGizmoWorldSpace ? ICON_FA_GLOBE : ICON_FA_CUBE;
	const char* spaceTip = scene.m_bGizmoWorldSpace ? "World space (click for Local)" : "Local space (click for World)";
	if (IconButton("##gizmo_space", spaceIcon, ImVec2(btnSize, btnSize), false))
		scene.m_bGizmoWorldSpace = !scene.m_bGizmoWorldSpace;
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("%s", spaceTip);

	//-----------------------------------------------------------------------------------
	//-- Camera manipulation buttons
	//-----------------------------------------------------------------------------------
	CCamera* camera = scene.Camera();
	std::string camera_tip;

	//-- change camera projection
	std::string proj_text = camera->m_mode == eCameraProjMode::ePerspective ? 
		ICON_FA_CUBE "  Perspective" : ICON_FA_SQUARE "  Orthographic";

	float proj_text_sx = ImGui::CalcTextSize(proj_text.c_str()).x + 20.f;

	btnPos.x = btnPos.x - proj_text_sx - kMargin;
	ImGui::SetCursorScreenPos(btnPos);
	
	//if (IconButton("##proj_mode", ICON_FA_CUBE, ImVec2(btnSize, btnSize),  false)) 
	if(ImGui::Button(proj_text.c_str(), ImVec2(proj_text_sx, btnSize)))
	{
		camera->m_mode = camera->m_mode == eCameraProjMode::ePerspective ?
			eCameraProjMode::eOrthographic : eCameraProjMode::ePerspective;
	}

	camera_tip = camera->m_mode == eCameraProjMode::ePerspective ?
		"Perspective (click for Orthographic)" : "Orthographic (click for Perspective)";

	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("%s", camera_tip.c_str());
	//-----------------------------------------------------------------------------------

	//-- change camera control mode
	std::string control_text = camera->m_controlMode == eCameraControlMode::eFreeLook ?
		ICON_FA_CAMERA_RETRO " FreeLook" : ICON_FA_CAMERA_ROTATE "  Orbital";

	float control_text_sx = ImGui::CalcTextSize(control_text.c_str()).x + 20.f;

	btnPos.x = btnPos.x - control_text_sx - kMargin;
	ImGui::SetCursorScreenPos(btnPos);
	if (ImGui::Button(control_text.c_str(), ImVec2(control_text_sx, btnSize)))
	{
		camera->SetControlMode(
			camera->m_controlMode == eCameraControlMode::eFreeLook ?
			eCameraControlMode::eOrbital : eCameraControlMode::eFreeLook
		);
	}

	if (camera->m_controlMode == eCameraControlMode::eFreeLook) {
		camera_tip = "FreeLook camera mode (F5 for relaod)\n\n"
			ICON_FA_CIRCLE_INFO "  Navigation:\n\n"
			ICON_FA_UP_DOWN_LEFT_RIGHT "  W/S/A/D -> Move\n\n"
			ICON_FA_ARROWS_UP_DOWN "  Space / Ctrl->Up / Down\n\n"
			ICON_FA_ROTATE "  Middle MB->Look";
	}
	else {
		camera_tip = "Orbital camera mode (F5 for relaod)\n\n"
			ICON_FA_CIRCLE_INFO "  Navigation:\n\n"
			ICON_FA_ROTATE "  Middle MB -> Rotate\n\n"
			ICON_FA_ARROWS_UP_DOWN "  Scroll -> Depth\n\n"
			ICON_FA_UP_DOWN_LEFT_RIGHT "  LShift + MMB -> Move target\n\n"
			ICON_FA_CIRCLE_INFO "  LShift only -> Display target cube\n\n";
	}
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("%s", camera_tip.c_str());
	//-----------------------------------------------------------------------------------
}

//-------------------------------------------------------------------------
//-- DrawStatsBadge for any additional vp info
//-------------------------------------------------------------------------
void CUIViewPort::DrawStatsBadge(ImVec2 imageOriginScreen, ImVec2 vpSize)
{
	CCamera* camera = CScene::Get().Camera();
	std::string line1 = std::format("{}  {:.0f} x {:.0f}", ICON_FA_UP_DOWN_LEFT_RIGHT, vpSize.x, vpSize.y);
	std::string line2 = std::format("{}  {:.2f}, {:.2f}, {:.2f}", ICON_FA_CAMERA,
		camera->m_position.x, camera->m_position.y, camera->m_position.z);

	constexpr float kPad = 8.0f;  // child's own inner padding
	constexpr float kGap = 4.0f;  // spacing between the two lines

	ImVec2 textSize1 = ImGui::CalcTextSize(line1.c_str());
	ImVec2 textSize2 = ImGui::CalcTextSize(line2.c_str());
	float w = (textSize1.x > textSize2.x ? textSize1.x : textSize2.x) + kPad * 2.0f;
	float h = textSize1.y + textSize2.y + kGap + kPad * 2.0f;

	ImVec2 pos(imageOriginScreen.x + kMargin, imageOriginScreen.y + vpSize.y - h - kMargin);

	ImGui::SetCursorScreenPos(pos);
	ImGui::PushStyleColor(ImGuiCol_ChildBg, kCardBg);
	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 6.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(kPad, kPad));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, kGap));
	ImGui::BeginChild("##vp_stats", ImVec2(w, h), true,
		ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
	ImGui::TextUnformatted(line1.c_str());
	ImGui::TextUnformatted(line2.c_str());
	ImGui::EndChild();
	ImGui::PopStyleVar(3);
	ImGui::PopStyleColor();
}

//-------------------------------------------------------------------------
//-- DrawGizmo
//-- Draws the ImGuizmo manipulator over the currently selected model,
//-- driven by the Select/Move/Rotate/Scale toolbar (scene.m_GizmoMode) and
//-- the World/Local toggle.
//-------------------------------------------------------------------------
void CUIViewPort::DrawGizmo(ImVec2 imageOriginScreen, ImVec2 vpSize)
{
	CScene& scene = CScene::Get();
	CModel* model = scene.m_SelectedModel;
	CCamera* camera = scene.Camera();

	if (!model || scene.m_GizmoMode == eGizmoMode::eSelect)
		return;

	ImGuizmo::SetOrthographic(camera->m_mode == eCameraProjMode::eOrthographic);
	ImGuizmo::SetDrawlist();
	ImGuizmo::SetRect(imageOriginScreen.x, imageOriginScreen.y, vpSize.x, vpSize.y);

	//-- DirectXMath and ImGuizmo's matrix_t both use row-vector layout
	//-- (row0=right, row1=up, row2=dir/forward, row3=position; v' = v*M),
	//-- so view/projection are handed over as-is — no transpose needed.
	DirectX::XMFLOAT4X4 viewF, projF;
	DirectX::XMStoreFloat4x4(&viewF, camera->GetViewMatrix());
	DirectX::XMStoreFloat4x4(&projF, camera->GetProjectionMatrix(vpSize.x / vpSize.y));

	float t[3] = { model->m_Position.x, model->m_Position.y, model->m_Position.z };
	float r[3] = {
		DirectX::XMConvertToDegrees(model->m_Rotation.x),
		DirectX::XMConvertToDegrees(model->m_Rotation.y),
		DirectX::XMConvertToDegrees(model->m_Rotation.z) };
	float s[3] = { model->m_Scale.x, model->m_Scale.y, model->m_Scale.z };

	float matrix[16];
	ImGuizmo::RecomposeMatrixFromComponents(t, r, s, matrix);

	ImGuizmo::OPERATION op = ImGuizmo::TRANSLATE;
	switch (scene.m_GizmoMode) {
	case eGizmoMode::eTranslate: op = ImGuizmo::TRANSLATE; break;
	case eGizmoMode::eRotate:    op = ImGuizmo::ROTATE; break;
	case eGizmoMode::eScale:     op = ImGuizmo::SCALE; break;
	default: break;
	}
	ImGuizmo::MODE gmode = scene.m_bGizmoWorldSpace ? ImGuizmo::WORLD : ImGuizmo::LOCAL;

	if (ImGuizmo::Manipulate((float*)&viewF, (float*)&projF, op, gmode, matrix))
	{
		ImGuizmo::DecomposeMatrixToComponents(matrix, t, r, s);

		model->m_Position = { t[0], t[1], t[2] };
		model->m_Rotation = {
			DirectX::XMConvertToRadians(r[0]),
			DirectX::XMConvertToRadians(r[1]),
			DirectX::XMConvertToRadians(r[2]) };
		model->m_Scale = { s[0], s[1], s[2] };
		model->UpdateTransform();
	}
}

//-------------------------------------------------------------------------
//-- DrawViewGizmo
//-- Blender/Unity-style orientation cube in the viewport corner
//-------------------------------------------------------------------------
void CUIViewPort::DrawViewGizmo(ImVec2 imageOriginScreen, ImVec2 vpSize)
{
	constexpr float kGizmoSize = 88.0f;
	constexpr float kOrbitLength = 8.0f;

	CCamera* camera = CScene::Get().Camera();

	ImVec2 pos(
		imageOriginScreen.x + vpSize.x - kGizmoSize - kMargin,
		imageOriginScreen.y + vpSize.y - kGizmoSize - kMargin);

	ImGuizmo::SetDrawlist();
	ImGuizmo::SetRect(imageOriginScreen.x, imageOriginScreen.y, vpSize.x, vpSize.y);

	DirectX::XMFLOAT4X4 viewF;
	DirectX::XMStoreFloat4x4(&viewF, camera->GetViewMatrix());

	ImGuizmo::ViewManipulate((float*)&viewF, kOrbitLength, pos, ImVec2(kGizmoSize, kGizmoSize), IM_COL32(0, 0, 0, 0));

	if (!ImGuizmo::IsUsingViewManipulate())
		return;

	// decode the (possibly modified) view back into eye position + yaw/pitch
	DirectX::XMMATRIX viewInv = DirectX::XMMatrixInverse(nullptr, DirectX::XMLoadFloat4x4(&viewF));
	DirectX::XMFLOAT4X4 viewInvF;
	DirectX::XMStoreFloat4x4(&viewInvF, viewInv);

	DirectX::XMFLOAT3 forward(viewInvF.m[2][0], viewInvF.m[2][1], viewInvF.m[2][2]);
	float len = sqrtf(forward.x * forward.x + forward.y * forward.y + forward.z * forward.z);
	if (len > 1e-5f) { forward.x /= len; forward.y /= len; forward.z /= len; }

	camera->m_position = { viewInvF.m[3][0], viewInvF.m[3][1], viewInvF.m[3][2] };
	camera->m_yaw = atan2f(forward.x, forward.z);
	camera->m_pitch = std::clamp(asinf(std::clamp(forward.y, -1.0f, 1.0f)),
		-DirectX::XM_PIDIV2 + 0.01f, DirectX::XM_PIDIV2 - 0.01f);
}


void CUIViewPort::RenderContent()
{
	UpdateInput();

	CHW& hw = CHW::Get();
	CRenderer& renderer = CRenderer::Get();
	CRenderTarget* rt = renderer.GetMainRT();

	ImVec2 size = ImGui::GetContentRegionAvail();
	if (size.x > 0 && size.y > 0)
	{
		if ((UINT)size.x != rt->m_Width || (UINT)size.y != rt->m_Height) {
			rt->Create(hw.m_Device, (UINT)size.x, (UINT)size.y);
			renderer.Resize((UINT)size.x, (UINT)size.y);

			//-- why not
			CScene::Get().Camera()->m_aspectRatio = size.x / size.y;
		}
	}

	//-- capture the image's screen-space origin *before* drawing it, so
	//-- every overlay below anchors to the same reference point.
	ImVec2 imageOrigin = ImGui::GetCursorScreenPos();

	//-- having orbital control mode -> draw debug box with target position
	auto camera = CScene::Get().Camera();
	if (camera && camera->m_LShitft) {
		DirectX::XMFLOAT4 color(1.f, 1.f, 0.f, 1.f);

		//-- same cube size
		float cube_size = dxfloat3_dist_to(camera->GetPosition(), camera->GetTarget()) * 
			tan(camera->m_fov / 2) * 0.05f;

		renderer.DebugRenderer()->SetRenderPhase(eDebugPhase::ePhaseAfterScene);
		renderer.DebugRenderer()->DrawCube(hw.m_Context, camera->m_target, cube_size, color);
	}

	renderer.Render();
	if (rt->m_SRV)
		ImGui::Image(rt->m_SRV, size);

	//-- 3D manipulator over the selected model, drawn right on the image
	if (size.x > 0 && size.y > 0)
		DrawGizmo(imageOrigin, size);

	//-- camera gizmo manipulator
	if (size.x > 220.0f && size.y > 160.0f)
		DrawViewGizmo(imageOrigin, size);

	//-- floating widgets, drawn on top of the rendered image -----------
	if (size.x > 140.0f && size.y > 40.0f)
		DrawToolbar(imageOrigin);

	if (size.x > 260.0f)
		DrawOverlaysButton(imageOrigin, size.x);

	if (size.x > 160.0f && size.y > 90.0f)
		DrawStatsBadge(imageOrigin, size);
}

//----------------------------------------------------------------------------
//-- Check Input for Camera and Viewport (trash but i`m lazy)
//----------------------------------------------------------------------------
void CUIViewPort::UpdateInput() {
	CScene& scene = CScene::Get();
	CCamera* camera = scene.Camera();

	if (!ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows)) {
		camera->m_moveForward = false;
		camera->m_moveBackward = false;
		camera->m_moveLeft = false;
		camera->m_moveRight = false;
		camera->m_moveUp = false;
		camera->m_moveDown = false;
		camera->m_MouseMB = false;
		camera->m_LShitft = false;
		return;
	}

	//-- reload camera
	if (ImGui::IsKeyPressed(ImGuiKey_F5)) {
		camera->SetControlMode(camera->m_controlMode);
		return;
	}

	//-- zoom / speed
	ImGuiIO& io = ImGui::GetIO();
	if (io.MouseWheel)
		camera->OnMouseWheel((int)io.MouseWheel);

	//-- camera rotation
	if (io.MouseDelta.x || io.MouseDelta.y)
		camera->OnMouseMove((int)io.MouseDelta.x, (int)io.MouseDelta.y);

	//-- moving camera in free look mode
	if (camera->m_controlMode == eCameraControlMode::eFreeLook)
	{
		if (camera->m_mode != eCameraProjMode::eOrthographic) {
			if (ImGui::IsKeyPressed(ImGuiKey_W)) camera->m_moveForward = true;
			else if (ImGui::IsKeyPressed(ImGuiKey_S)) camera->m_moveBackward = true;
			else if (ImGui::IsKeyPressed(ImGuiKey_A)) camera->m_moveLeft = true;
			else if (ImGui::IsKeyPressed(ImGuiKey_D)) camera->m_moveRight = true;
			else if (ImGui::IsKeyPressed(ImGuiKey_Space)) camera->m_moveUp = true;
			else if (ImGui::IsKeyPressed(ImGuiKey_LeftCtrl)) camera->m_moveDown = true;
		}

		if (ImGui::IsKeyReleased(ImGuiKey_W)) camera->m_moveForward = false;
		else if (ImGui::IsKeyReleased(ImGuiKey_S)) camera->m_moveBackward = false;
		else if (ImGui::IsKeyReleased(ImGuiKey_A)) camera->m_moveLeft = false;
		else if (ImGui::IsKeyReleased(ImGuiKey_D)) camera->m_moveRight = false;
		else if (ImGui::IsKeyReleased(ImGuiKey_Space)) camera->m_moveUp = false;
		else if (ImGui::IsKeyReleased(ImGuiKey_LeftCtrl)) camera->m_moveDown = false;
	}
	else {
		if (ImGui::IsKeyPressed(ImGuiKey_LeftShift)) camera->m_LShitft = true;
		if (ImGui::IsKeyReleased(ImGuiKey_LeftShift)) camera->m_LShitft = false;
	}

	if (ImGui::IsMouseClicked(ImGuiMouseButton_Middle)) camera->m_MouseMB = true;
	if (ImGui::IsMouseReleased(ImGuiMouseButton_Middle)) camera->m_MouseMB = false;

	//-- gizmo mode
	if (ImGui::IsKeyPressed(ImGuiKey_Q))
		scene.m_GizmoMode = eGizmoMode::eSelect;

	if (ImGui::IsKeyPressed(ImGuiKey_W))
		scene.m_GizmoMode = eGizmoMode::eTranslate;

	if (ImGui::IsKeyPressed(ImGuiKey_E))
		scene.m_GizmoMode = eGizmoMode::eRotate;

	if (ImGui::IsKeyPressed(ImGuiKey_R))
		scene.m_GizmoMode = eGizmoMode::eScale;
}
//----------------------------------------------------------------------------
