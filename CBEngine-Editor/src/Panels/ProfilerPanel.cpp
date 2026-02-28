#include "ProfilerPanel.h"

#include "CBEngine/Debug/Instrumentor.h"
#include "imgui.h"

#include <unordered_map>
#include <string>
#include <algorithm>
#include <ctime>
#include <filesystem>

namespace CB
{
	// Category colors for visual distinction
	static ImVec4 GetCategoryColor(const char* category)
	{
		// Hash the first char for fast lookup
		switch (category[0]) {
			case 'P': return ImVec4(0.3f, 0.6f, 1.0f, 1.0f);  // Physics - blue
			case 'R': return ImVec4(0.4f, 0.9f, 0.4f, 1.0f);  // Rendering - green
			case 'V': return ImVec4(1.0f, 0.6f, 0.2f, 1.0f);  // Voxel - orange
			case 'S':
				if (category[1] == 'c') return ImVec4(0.9f, 0.9f, 0.3f, 1.0f); // Scene - yellow
				return ImVec4(0.8f, 0.5f, 0.9f, 1.0f); // Script - purple
			case 'E': return ImVec4(0.7f, 0.7f, 0.7f, 1.0f);  // Editor - gray
			default:  return ImVec4(0.6f, 0.6f, 0.6f, 1.0f);   // General - dim gray
		}
	}

	static ImVec4 GetTimeColor(float timeMs)
	{
		if (timeMs < 0.5f)  return ImVec4(0.5f, 0.5f, 0.5f, 1.0f); // Gray
		if (timeMs < 2.0f)  return ImVec4(0.0f, 1.0f, 0.0f, 1.0f); // Green
		if (timeMs < 5.0f)  return ImVec4(1.0f, 1.0f, 0.0f, 1.0f); // Yellow
		if (timeMs < 10.0f) return ImVec4(1.0f, 0.5f, 0.0f, 1.0f); // Orange
		return ImVec4(1.0f, 0.2f, 0.2f, 1.0f);                      // Red
	}

	void ProfilerPanel::OnImGuiRender()
	{
		if (!m_Visible)
			return;

		auto& instrumentor = Instrumentor::Get();
		float fps = instrumentor.GetFPS();
		float frameTimeMs = instrumentor.GetFrameTimeMs();
		const auto& frameHistory = instrumentor.GetFrameTimeHistory();
		size_t historyIndex = instrumentor.GetFrameTimeIndex();

		ImGui::SetNextWindowSize(ImVec2(450, 600), ImGuiCond_FirstUseEver);
		if (ImGui::Begin(m_Name.c_str(), &m_Visible)) {
			// FPS display with color coding
			ImVec4 fpsColor;
			if (fps >= 55.0f)
				fpsColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
			else if (fps >= 30.0f)
				fpsColor = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
			else
				fpsColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);

			ImGui::TextColored(fpsColor, "FPS: %.1f", fps);
			ImGui::SameLine();
			ImGui::Text("(%.2f ms)", frameTimeMs);

			ImGui::Separator();

			// --- Recording controls ---
			RenderRecordingControls();

			ImGui::Separator();

			// Frame time history graph
			ImGui::Text("Frame Time History (120 frames)");

			float orderedHistory[FRAME_HISTORY_SIZE];
			for (size_t i = 0; i < FRAME_HISTORY_SIZE; i++) {
				size_t idx = (historyIndex + i) % FRAME_HISTORY_SIZE;
				orderedHistory[i] = frameHistory[idx];
			}

			float maxTime = 0.0f;
			for (size_t i = 0; i < FRAME_HISTORY_SIZE; i++) {
				if (orderedHistory[i] > maxTime)
					maxTime = orderedHistory[i];
			}
			if (maxTime < 16.67f) maxTime = 16.67f;
			maxTime *= 1.1f;

			char overlay[64];
			snprintf(overlay, sizeof(overlay), "%.2f ms", frameTimeMs);
			ImGui::PlotLines("##FrameTime", orderedHistory, FRAME_HISTORY_SIZE, 0, overlay,
			                 0.0f, maxTime, ImVec2(0, 80));

			ImGui::TextDisabled("Target: 16.67ms (60fps) | 33.33ms (30fps)");

			ImGui::Separator();

			// Category breakdown
			RenderScopeTree();
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
		if (e.GetKeyCode() == CB_KEY_F3 && e.GetRepeatCount() == 0) {
			ToggleVisibility();
			return true;
		}
		return false;
	}

	void ProfilerPanel::RenderRecordingControls()
	{
		auto& instrumentor = Instrumentor::Get();
		bool recording = instrumentor.IsRecording();

		if (recording) {
			// Pulsing red circle indicator
			float time = static_cast<float>(ImGui::GetTime());
			float alpha = 0.5f + 0.5f * sinf(time * 4.0f);
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.2f, 0.2f, alpha));
			ImGui::Bullet();
			ImGui::PopStyleColor();
			ImGui::SameLine();
			ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Recording...");
			ImGui::SameLine();

			// Show elapsed time and frame count
			float elapsed = std::chrono::duration<float, std::milli>(
				std::chrono::steady_clock::now() - std::chrono::steady_clock::now()).count();
			ImGui::Text("(%u frames)", instrumentor.GetRecordedFrameCount());

			ImGui::SameLine();
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
			if (ImGui::Button("Stop")) {
				instrumentor.StopRecording();
			}
			ImGui::PopStyleColor(2);
		}
		else {
			// Record button
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.8f, 0.3f, 1.0f));
			if (ImGui::Button("Record")) {
				instrumentor.StartRecording();
			}
			ImGui::PopStyleColor(2);

			// Show last recording stats and export button if we have data
			const auto& recorded = instrumentor.GetRecordedScopes();
			if (!recorded.empty()) {
				ImGui::SameLine();
				float durationMs = instrumentor.GetRecordDurationMs();
				uint32_t frameCount = instrumentor.GetRecordedFrameCount();

				float durationSec = durationMs / 1000.0f;
				ImGui::Text("Last: %.1fs, %u frames, %zu scopes",
				            durationSec, frameCount, recorded.size());

				ImGui::SameLine();
				if (ImGui::Button("Export")) {
					// Generate timestamped filename
					std::time_t now = std::time(nullptr);
					std::tm tm;
					localtime_s(&tm, &now);
					char filename[128];
					snprintf(filename, sizeof(filename), "profiler_%04d%02d%02d_%02d%02d%02d.json",
					         tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
					         tm.tm_hour, tm.tm_min, tm.tm_sec);

					if (instrumentor.ExportRecording(filename)) {
						m_ExportMessage = "Exported: ";
						m_ExportMessage += filename;
						m_ExportMessageTimer = 3.0f;
					}
					else {
						m_ExportMessage = "Export failed!";
						m_ExportMessageTimer = 3.0f;
					}
				}

				// Show recording summary
				if (!instrumentor.GetRecordedFrameTimes().empty()) {
					const auto& frameTimes = instrumentor.GetRecordedFrameTimes();
					float avgMs = 0.0f;
					float minMs = 999999.0f;
					float maxMs = 0.0f;
					for (float ft : frameTimes) {
						avgMs += ft;
						if (ft < minMs) minMs = ft;
						if (ft > maxMs) maxMs = ft;
					}
					avgMs /= static_cast<float>(frameTimes.size());

					ImGui::TextDisabled("Avg: %.2fms | Min: %.2fms | Max: %.2fms | Avg FPS: %.1f",
					                    avgMs, minMs, maxMs, 1000.0f / avgMs);
				}
			}

			// Export success/failure message with fade
			if (m_ExportMessageTimer > 0.0f) {
				float alpha = m_ExportMessageTimer > 1.0f ? 1.0f : m_ExportMessageTimer;
				ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, alpha), "%s", m_ExportMessage.c_str());
				m_ExportMessageTimer -= ImGui::GetIO().DeltaTime;
			}
		}
	}

	void ProfilerPanel::RenderScopeTree()
	{
		const auto& scopes = Instrumentor::Get().GetCurrentFrameScopes();

		if (scopes.empty()) {
			ImGui::TextDisabled("No profiling data available");
			return;
		}

		// Group scopes by category and compute totals
		struct CategoryInfo {
			float TotalMs = 0.0f;
			std::vector<const ProfileResult*> Scopes;
		};
		// Use ordered map-like approach with fixed category order
		const char* categoryOrder[] = { "Physics", "Rendering", "Voxel", "Scene", "Script", "Editor", "General" };
		constexpr int numCategories = 7;
		CategoryInfo categories[numCategories];

		for (const auto& scope : scopes) {
			float timeMs = scope.ElapsedTime.count() / 1000.0f;
			int catIdx = numCategories - 1; // default to General
			for (int i = 0; i < numCategories; i++) {
				if (strcmp(scope.Category, categoryOrder[i]) == 0) {
					catIdx = i;
					break;
				}
			}
			categories[catIdx].TotalMs += timeMs;
			categories[catIdx].Scopes.push_back(&scope);
		}

		ImGui::BeginChild("ScopeTree", ImVec2(0, 0), false);

		for (int c = 0; c < numCategories; c++) {
			auto& cat = categories[c];
			if (cat.Scopes.empty()) continue;

			ImVec4 catColor = GetCategoryColor(categoryOrder[c]);

			// Category header with total time
			ImGui::PushStyleColor(ImGuiCol_Text, catColor);
			char header[128];
			snprintf(header, sizeof(header), "%s (%.2f ms)###cat%d", categoryOrder[c], cat.TotalMs, c);
			bool open = ImGui::CollapsingHeader(header, ImGuiTreeNodeFlags_DefaultOpen);
			ImGui::PopStyleColor();

			if (open) {
				ImGui::Indent(12.0f);

				// Sort scopes by time descending within each category
				std::vector<const ProfileResult*> sorted = cat.Scopes;
				std::sort(sorted.begin(), sorted.end(), [](const ProfileResult* a, const ProfileResult* b) {
					return a->ElapsedTime > b->ElapsedTime;
				});

				for (const auto* scope : sorted) {
					float timeMs = scope->ElapsedTime.count() / 1000.0f;
					if (timeMs < 0.01f) continue; // skip sub-microsecond noise

					ImVec4 timeColor = GetTimeColor(timeMs);
					ImGui::TextColored(timeColor, "%6.2f ms", timeMs);
					ImGui::SameLine();
					ImGui::TextUnformatted(scope->Name.c_str());
				}

				ImGui::Unindent(12.0f);
			}
		}

		ImGui::EndChild();
	}
}
