#pragma once

#include "imgui.h"
#include "imgui_internal.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace CB
{
	struct ComponentCard
	{
		struct MenuAction
		{
			const char* Label;
			bool* Triggered;
		};

		// Begin a component card. Returns true if the content area is visible (expanded).
		// Set removable=false for components that cannot be removed (e.g. Transform).
		// removed/reset are output booleans set when the user clicks those menu items.
		static bool Begin(const char* label,
		                  bool removable = true,
		                  bool* removed = nullptr,
		                  bool* reset = nullptr,
		                  const std::vector<MenuAction>& extraActions = {})
		{
			ImGui::PushID(label);

			// Track collapse state per ImGui ID
			ImGuiID id = ImGui::GetID("##CardCollapse");
			if (s_CollapseState.find(id) == s_CollapseState.end())
				s_CollapseState[id] = true; // default expanded

			bool& expanded = s_CollapseState[id];
			s_CurrentExpanded = expanded;

			float availWidth = ImGui::GetContentRegionAvail().x;

			// Begin group to capture card bounds
			// Split draw channels so we can draw background behind content
			ImGui::GetWindowDrawList()->ChannelsSplit(2);
			ImGui::GetWindowDrawList()->ChannelsSetCurrent(1); // Content on channel 1
			ImGui::BeginGroup();

			// --- Header ---
			float headerHeight = ImGui::GetFrameHeight();
			float menuButtonWidth = headerHeight;

			// Header background
			ImVec2 headerMin = ImGui::GetCursorScreenPos();
			auto headerMax = ImVec2(headerMin.x + availWidth, headerMin.y + headerHeight);
			ImU32 headerColor = expanded
				                    ? ImGui::GetColorU32(ImVec4(0.20f, 0.20f, 0.24f, 1.0f))
				                    : ImGui::GetColorU32(ImVec4(0.17f, 0.17f, 0.20f, 1.0f));
			ImGui::GetWindowDrawList()->AddRectFilled(headerMin, headerMax, headerColor, 4.0f);

			// Collapse arrow + label (clickable area)
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1, 1, 1, 0.05f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1, 1, 1, 0.1f));

			const char* arrow = expanded ? "v" : ">";
			char headerLabel[256];
			snprintf(headerLabel, sizeof(headerLabel), " %s  %s", arrow, label);

			if (ImGui::Button(headerLabel, ImVec2(availWidth - menuButtonWidth - 4.0f, headerHeight))) {
				expanded = !expanded;
			}

			ImGui::PopStyleColor(3);

			// 3-dot menu button on the same line
			ImGui::SameLine(0.0f, 2.0f);

			char menuBtnId[64];
			snprintf(menuBtnId, sizeof(menuBtnId), "...##%s_menu", label);

			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1, 1, 1, 0.1f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1, 1, 1, 0.15f));

			if (ImGui::Button(menuBtnId, ImVec2(menuButtonWidth, headerHeight))) {
				ImGui::OpenPopup("##ComponentContextMenu");
			}

			ImGui::PopStyleColor(3);

			// Context menu popup
			if (ImGui::BeginPopup("##ComponentContextMenu")) {
				// Extra actions first
				for (const auto& action : extraActions) {
					if (ImGui::MenuItem(action.Label)) { if (action.Triggered) *action.Triggered = true; }
				}

				if (!extraActions.empty())
					ImGui::Separator();

				// Reset
				if (ImGui::MenuItem("Reset to Default")) { if (reset) *reset = true; }

				ImGui::Separator();

				// Remove (only if removable)
				if (removable) { if (ImGui::MenuItem("Remove Component")) { if (removed) *removed = true; } }
				else {
					ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.4f);
					ImGui::MenuItem("Remove Component", nullptr, false, false);
					ImGui::PopStyleVar();
				}

				ImGui::EndPopup();
			}

			// Content area
			if (expanded) {
				ImGui::Indent(4.0f);
				ImGui::Spacing();
				return true;
			}

			return false;
		}

		static void End()
		{
			if (s_CurrentExpanded) {
				ImGui::Spacing();
				ImGui::Unindent(4.0f);
			}

			ImGui::EndGroup();

			// Draw card border around the group
			ImVec2 groupMin = ImGui::GetItemRectMin();
			ImVec2 groupMax = ImGui::GetItemRectMax();

			// Add some padding to the rect
			groupMin.x -= 2.0f;
			groupMin.y -= 2.0f;
			groupMax.x += 2.0f;
			groupMax.y += 2.0f;

			ImU32 borderColor = ImGui::GetColorU32(ImVec4(0.25f, 0.25f, 0.28f, 1.0f));
			ImU32 bgColor = ImGui::GetColorU32(ImVec4(0.14f, 0.14f, 0.16f, 1.0f));

			// Draw background on channel 0 (behind content), then merge
			ImDrawList* drawList = ImGui::GetWindowDrawList();
			drawList->ChannelsSetCurrent(0); // Background channel
			drawList->AddRectFilled(groupMin, groupMax, bgColor, 4.0f);
			drawList->AddRect(groupMin, groupMax, borderColor, 4.0f);
			drawList->ChannelsMerge();

			ImGui::PopID();
			ImGui::Spacing();
		}
	private:
		static inline std::unordered_map<ImGuiID, bool> s_CollapseState;
		static inline bool s_CurrentExpanded = false;
	};
}