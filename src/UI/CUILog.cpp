//----------------------------------------------------------------------------
//-- CUILog: Log window for ImGui Docking
//----------------------------------------------------------------------------
#include "CUILog.h"
#include "../Core/CLog.h"

void CUILog::RenderContent()
{
    CLog& log = CLog::Get();

    if (ImGui::Button("Clear"))
        log.Clear();

    ImGui::SameLine();
    ImGui::Checkbox("Auto-scroll", &m_AutoScroll); //-- simple imgui window class needed
    ImGui::SameLine();
    ImGui::Checkbox("Info", &m_ShowInfo);
    ImGui::SameLine();
    ImGui::Checkbox("Warning", &m_ShowWarning);
    ImGui::SameLine();
    ImGui::Checkbox("Error", &m_ShowError);
    ImGui::SameLine();
    ImGui::Checkbox("Debug", &m_ShowDebug);

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
        switch (entry.type) {
        case eLogLevel::WARNING: color = ImVec4(1.0f, 0.8f, 0.2f, 1.0f); break;
        case eLogLevel::ERR:     color = ImVec4(1.0f, 0.3f, 0.3f, 1.0f); break;
        case eLogLevel::DEBUG:   color = ImVec4(0.6f, 0.6f, 0.6f, 1.0f); break;
        default:      color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); break;
        }

        ImGui::TextColored(color, "[%s] [%s] %s",
            entry.timestamp.c_str(),
            log.LevelToString(entry.type),
            entry.message.c_str());
    }

    if (m_AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        ImGui::SetScrollHereY(1.0f);

    ImGui::EndChild();
}
