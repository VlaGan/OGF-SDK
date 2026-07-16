//----------------------------------------------------------------------------
//-- CUILog: Log window for ImGui Docking
//----------------------------------------------------------------------------
#include "CUILog.h"
#include "../Core/CLog.h"
#include <fontawesome/IconsFontAwesome6.h>

void CUILog::RenderContent()
{
    CLog& log = CLog::Get();

    if (ImGui::Button(ICON_FA_BROOM "  Clear"))
        log.Clear();

    ImGui::SameLine();
    ImGui::Checkbox(ICON_FA_ARROWS_DOWN_TO_LINE "  Auto-scroll", &m_AutoScroll); //-- simple imgui window class needed
    ImGui::SameLine();
    ImGui::Checkbox(ICON_FA_CIRCLE_INFO "  Info", &m_ShowInfo);
    ImGui::SameLine();
    ImGui::Checkbox(ICON_FA_TRIANGLE_EXCLAMATION "  Warning", &m_ShowWarning);
    ImGui::SameLine();
    ImGui::Checkbox(ICON_FA_CIRCLE_XMARK "  Error", &m_ShowError);
    ImGui::SameLine();
    ImGui::Checkbox(ICON_FA_BUG "  Debug", &m_ShowDebug);

    ImGui::Separator();

    ImGui::BeginChild("LogScrollRegion", ImVec2(0, 0), false,
        ImGuiWindowFlags_HorizontalScrollbar);

    for (const auto& entry : log.GetEntries()) {
        switch (entry.type) {
        case eLogLevel::INFO:    if (!m_ShowInfo) continue; break;
        case eLogLevel::WARNING: if (!m_ShowWarning) continue; break;
        case eLogLevel::ERR:     if (!m_ShowError) continue; break;
        case eLogLevel::DEBUG:   if (!m_ShowDebug) continue; break;
        }

        ImVec4 color;
        const char* icon;
        switch (entry.type) {
        case eLogLevel::WARNING: color = ImVec4(1.0f, 0.8f, 0.2f, 1.0f); icon = ICON_FA_TRIANGLE_EXCLAMATION; break;
        case eLogLevel::ERR:     color = ImVec4(1.0f, 0.3f, 0.3f, 1.0f); icon = ICON_FA_CIRCLE_XMARK; break;
        case eLogLevel::DEBUG:   color = ImVec4(0.6f, 0.6f, 0.6f, 1.0f); icon = ICON_FA_BUG; break;
        default:      color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); icon = ICON_FA_CIRCLE_INFO; break;
        }

        ImGui::TextColored(color, "%s [%s] [%s] %s",
            icon,
            entry.timestamp.c_str(),
            log.LevelToString(entry.type),
            entry.message.c_str());
    }

    if (m_AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        ImGui::SetScrollHereY(1.0f);

    ImGui::EndChild();
}
