#include "ConsolePanel.h"

#include "imgui.h"
#include "CBEngine/Core/LogBuffer.h"

namespace CB
{
    void ConsolePanel::OnImGuiRender()
    {
        if (!m_Visible)
            return;

        ImGui::Begin(m_Name.c_str(), &m_Visible);

        // Toolbar
        if (ImGui::Button("Clear"))
            LogBuffer::Get().Clear();

        ImGui::SameLine();
        ImGui::Checkbox("Auto-scroll", &m_AutoScroll);

        ImGui::SameLine();
        ImGui::Separator();
        ImGui::SameLine();

        // Filter buttons
        ImGui::Text("Show:");
        ImGui::SameLine();
        ImGui::Checkbox("Trace", &m_ShowTrace);
        ImGui::SameLine();
        ImGui::Checkbox("Info", &m_ShowInfo);
        ImGui::SameLine();
        ImGui::Checkbox("Warn", &m_ShowWarn);
        ImGui::SameLine();
        ImGui::Checkbox("Error", &m_ShowError);

        ImGui::Separator();

        // Log output
        ImGui::BeginChild("LogScrollRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

        const auto& messages = LogBuffer::Get().GetMessages();

        if (messages.empty()) { ImGui::TextDisabled("No log messages..."); }
        else
        {
            for (const auto& msg : messages)
            {
                // Filter by level
                bool show = false;
                auto color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

                switch (msg.Level)
                {
                case LogLevel::Trace:
                    show = m_ShowTrace;
                    color = ImVec4(0.5f, 0.5f, 0.5f, 1.0f); // Gray
                    break;
                case LogLevel::Info:
                    show = m_ShowInfo;
                    color = ImVec4(0.0f, 0.8f, 0.0f, 1.0f); // Green
                    break;
                case LogLevel::Warn:
                    show = m_ShowWarn;
                    color = ImVec4(1.0f, 0.8f, 0.0f, 1.0f); // Yellow
                    break;
                case LogLevel::Error:
                case LogLevel::Critical:
                    show = m_ShowError;
                    color = ImVec4(1.0f, 0.3f, 0.3f, 1.0f); // Red
                    break;
                case LogLevel::Debug:
                    show = m_ShowTrace;
                    color = ImVec4(0.0f, 0.8f, 0.0f, 1.0f); // Green
                    break;
                }

                if (show)
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, color);
                    ImGui::TextUnformatted(msg.Message.c_str());
                    ImGui::PopStyleColor();
                }
            }
        }

        if (m_AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            ImGui::SetScrollHereY(1.0f);

        ImGui::EndChild();

        ImGui::End();
    }
}
