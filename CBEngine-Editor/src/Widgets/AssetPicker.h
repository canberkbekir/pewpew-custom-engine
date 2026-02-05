#pragma once

#include "imgui.h"
#include "CBEngine/Core/Core.h"
#include "CBEngine/Core/UUID.h"
#include "CBEngine/Asset/AssetMetadata.h"
#include "CBEngine/Asset/AssetManager.h"

#include <string>
#include <algorithm>

namespace CB
{
	class AssetPicker
	{
	public:
		// Draw an asset picker field for a specific asset type
		// Returns true if an asset was selected, and sets outUUID to the selected asset
		static bool Draw(const char* label, UUID& currentUUID, AssetType filterType)
		{
			bool changed = false;

			ImGui::PushID(label);

			// Get current asset name for display
			String displayName = "None";
			if (currentUUID.IsValid())
			{
				const AssetMetadata* metadata = AssetManager::GetRegistry().GetMetadata(currentUUID);
				if (metadata)
				{
					displayName = metadata->FilePath.stem().string();
				}
			}

			// Draw the field
			ImGui::Text("%s", label);
			ImGui::SameLine();

			float buttonWidth = ImGui::GetContentRegionAvail().x - 25.0f;
			if (buttonWidth < 100.0f) buttonWidth = 100.0f;

			// Main button that opens the picker
			if (ImGui::Button(displayName.c_str(), ImVec2(buttonWidth, 0)))
			{
				ImGui::OpenPopup("AssetPickerPopup");
				s_SearchBuffer[0] = '\0';
			}

			// Clear button
			ImGui::SameLine();
			if (ImGui::SmallButton("X"))
			{
				currentUUID = UUID(0);
				changed = true;
			}

			// Also support drag-drop on the button
			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_UUID"))
				{
					UUID droppedUUID = *(UUID*)payload->Data;
					const AssetMetadata* metadata = AssetManager::GetRegistry().GetMetadata(droppedUUID);
					if (metadata && metadata->Type == filterType)
					{
						currentUUID = droppedUUID;
						changed = true;
					}
				}
				ImGui::EndDragDropTarget();
			}

			// The picker popup
			ImGui::SetNextWindowSize(ImVec2(300, 400), ImGuiCond_Appearing);
			if (ImGui::BeginPopup("AssetPickerPopup"))
			{
				// Search bar with auto-focus
				if (ImGui::IsWindowAppearing())
					ImGui::SetKeyboardFocusHere();

				ImGui::SetNextItemWidth(-1);
				ImGui::InputTextWithHint("##Search", "Search...", s_SearchBuffer, sizeof(s_SearchBuffer));

				ImGui::Separator();

				// "None" option
				if (ImGui::Selectable("None", !currentUUID.IsValid()))
				{
					currentUUID = UUID(0);
					changed = true;
					ImGui::CloseCurrentPopup();
				}

				ImGui::Separator();

				// List all assets of the specified type
				String searchLower = s_SearchBuffer;
				std::transform(searchLower.begin(), searchLower.end(), searchLower.begin(), ::tolower);

				ImGui::BeginChild("AssetList", ImVec2(0, 0), false);

				const auto& registry = AssetManager::GetRegistry();
				for (const auto& [uuid, metadata] : registry.GetAllAssets())
				{
					if (metadata.Type != filterType)
						continue;

					String assetName = metadata.FilePath.stem().string();

					// Apply search filter
					if (!searchLower.empty())
					{
						String nameLower = assetName;
						std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);
						if (nameLower.find(searchLower) == String::npos)
							continue;
					}

					bool isSelected = (uuid == currentUUID);

					// Show icon based on type
					const char* icon = GetIconForType(filterType);
					String displayLabel = String(icon) + " " + assetName;

					if (ImGui::Selectable(displayLabel.c_str(), isSelected))
					{
						currentUUID = uuid;
						changed = true;
						ImGui::CloseCurrentPopup();
					}

					// Tooltip with full path
					if (ImGui::IsItemHovered())
					{
						ImGui::BeginTooltip();
						ImGui::Text("%s", metadata.FilePath.string().c_str());
						ImGui::EndTooltip();
					}
				}

				ImGui::EndChild();
				ImGui::EndPopup();
			}

			ImGui::PopID();
			return changed;
		}

		// Convenience overloads that automatically determine the asset type
		static bool DrawMesh(const char* label, UUID& currentUUID)
		{
			return Draw(label, currentUUID, AssetType::Mesh);
		}

		static bool DrawTexture(const char* label, UUID& currentUUID)
		{
			return Draw(label, currentUUID, AssetType::Texture2D);
		}

		static bool DrawMaterial(const char* label, UUID& currentUUID)
		{
			return Draw(label, currentUUID, AssetType::Material);
		}

		static bool DrawShader(const char* label, UUID& currentUUID)
		{
			return Draw(label, currentUUID, AssetType::Shader);
		}

	private:
		static const char* GetIconForType(AssetType type)
		{
			switch (type)
			{
				case AssetType::Mesh:      return "[M]";
				case AssetType::Texture2D: return "[T]";
				case AssetType::Material:  return "[MAT]";
				case AssetType::Shader:    return "[S]";
				default:                   return "[?]";
			}
		}

		static inline char s_SearchBuffer[256] = "";
	};
}
