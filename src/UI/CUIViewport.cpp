//----------------------------------------------------------------------------
//-- CUIViewPort: Viewport window for ImGui Docking
//----------------------------------------------------------------------------
#include "CUIViewport.h"
#include <format>
#include "../_defines.h"
#include "../Render/CHW.h"
#include "../Render/CRenderer.h"
#include "../Core/CCamera.h"

extern CCamera m_Camera;
//-------------------------------------------------------------------------
//-- RenderContent
//-------------------------------------------------------------------------
void CUIViewPort::RenderContent()
{
    CHW& hw = CHW::Get();
    CRenderer& renderer = CRenderer::Get();
    CRenderTarget* rt = renderer.GetMainRT();

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

    ImGui::SetCursorPos(ImVec2(15, 45));
    ImGui::Text("VP size: [%.0f, %.0f]", size.x, size.y);
    ImGui::Text("Camera Pos: [%.2f, %.2f, %.2f]", VPUSH3(m_Camera.m_position));

    ImVec2 cursorPos = ImGui::GetCursorPos();
    std::string inputText = std::format("Move: W/S/A/D\nUp/Down: Space/Ctrl\nDirection: Mouse RB+LB");
    ImVec2 textSize = ImGui::CalcTextSize(inputText.c_str());
    ImGui::SetCursorPos(ImVec2(ImGui::GetContentRegionAvail().x - textSize.x, 45));
    ImGui::Text(inputText.c_str());
}
