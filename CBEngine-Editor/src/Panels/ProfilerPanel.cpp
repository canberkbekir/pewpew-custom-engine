#include "ProfilerPanel.h"

#include "CBEngine/Debug/Instrumentor.h"
#include "imgui.h"

namespace CB
{
    void ProfilerPanel::OnImGuiRender()
    {
        if (!m_Visible)
            return;

        auto& instrumentor = Instrumentor::Get();
        float fps = instrumentor.GetFPS();
        float frameTimeMs = instrumentor.GetFrameTimeMs();
        const auto& frameHistory = instrumentor.GetFrameTimeHistory();
        size_t historyIndex = instrumentor.GetFrameTimeIndex();

        ImGui::SetNextWindowSize(ImVec2(400, 350), ImGuiCond_FirstUseEver);
        if (ImGui::Begin(m_Name.c_str(), &m_Visible))
        {
            // FPS display with color coding
            ImVec4 fpsColor;
            if (fps >= 55.0f)
                fpsColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f); // Green
            else if (fps >= 30.0f)
                fpsColor = ImVec4(1.0f, 1.0f, 0.0f, 1.0f); // Yellow
            else
                fpsColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); // Red

            ImGui::TextColored(fpsColor, "FPS: %.1f", fps);
            ImGui::SameLine();
            ImGui::Text("(%.2f ms)", frameTimeMs);

            ImGui::Separator();

            // Frame time history graph
            ImGui::Text("Frame Time History (120 frames)");

            // Build ordered array for plotting (oldest to newest)
            float orderedHistory[FRAME_HISTORY_SIZE];
            for (size_t i = 0; i < FRAME_HISTORY_SIZE; i++)
            {
                size_t idx = (historyIndex + i) % FRAME_HISTORY_SIZE;
                orderedHistory[i] = frameHistory[idx];
            }

            // Calculate min/max for graph scaling
            float minTime = FLT_MAX, maxTime = 0.0f;
            for (size_t i = 0; i < FRAME_HISTORY_SIZE; i++)
            {
                if (orderedHistory[i] > 0.0f)
                {
                    minTime = std::min(minTime, orderedHistory[i]);
                    maxTime = std::max(maxTime, orderedHistory[i]);
                }
            }

            // Add some padding to max for better visualization
            if (maxTime < 16.67f) maxTime = 16.67f; // At least show 60fps line
            maxTime *= 1.1f;
            if (minTime == FLT_MAX) minTime = 0.0f;

            char overlay[64];
            snprintf(overlay, sizeof(overlay), "%.2f ms", frameTimeMs);
            ImGui::PlotLines("##FrameTime", orderedHistory, FRAME_HISTORY_SIZE, 0, overlay,
                             0.0f, maxTime, ImVec2(0, 80));

            // Reference lines text
            ImGui::TextDisabled("Target: 16.67ms (60fps) | 33.33ms (30fps)");

            ImGui::Separator();

            // Scope breakdown
            if (ImGui::CollapsingHeader("Scope Breakdown", ImGuiTreeNodeFlags_DefaultOpen)) { RenderScopeTree(); }
        }
        ImGui::End();
    }

    void ProfilerPanel::OnEvent(Event& e)
    {
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<KeyPressedEvent>(CB_BIND_EVENT_FN(ProfilerPanel::OnKeyPressed));
    }

    bool ProfilerPanel::OnKeyPressed(KeyPressedEvent& e)
    {
        if (e.GetKeyCode() == CB_KEY_F3 && e.GetRepeatCount() == 0)
        {
            ToggleVisibility();
            return true;
        }
        return false;
    }

    void ProfilerPanel::RenderScopeTree()
    {
        const auto& scopes = Instrumentor::Get().GetCurrentFrameScopes();

        if (scopes.empty())
        {
            ImGui::TextDisabled("No profiling data available");
            return;
        }

        ImGui::BeginChild("ScopeTree", ImVec2(0, 0), false);

        for (const auto& scope : scopes)
        {
            float timeMs = scope.ElapsedTime.count() / 1000.0f;

            // Color code by time spent
            ImVec4 color;
            if (timeMs < 1.0f)
                color = ImVec4(0.5f, 0.5f, 0.5f, 1.0f); // Gray - negligible
            else if (timeMs < 5.0f)
                color = ImVec4(0.0f, 1.0f, 0.0f, 1.0f); // Green - fast
            else if (timeMs < 10.0f)
                color = ImVec4(1.0f, 1.0f, 0.0f, 1.0f); // Yellow - moderate
            else
                color = ImVec4(1.0f, 0.5f, 0.0f, 1.0f); // Orange - slow

            ImGui::TextColored(color, "%.2f ms", timeMs);
            ImGui::SameLine();
            ImGui::Text("%s", scope.Name.c_str());
        }

        ImGui::EndChild();
    }
}
