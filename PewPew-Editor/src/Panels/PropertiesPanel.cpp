#include "PropertiesPanel.h"
#include "imgui.h"
#include "PewPew/Selection/Selection.h"
#include "PewPew/Asset/AssetManager.h"

// Editor widgets
#include "../Widgets/EntityEditor.h"
#include "../Widgets/MaterialEditor.h"
#include "../Widgets/TextureEditor.h"
#include "../Widgets/MeshEditor.h"
#include "../Widgets/ShaderEditor.h"

namespace PewPew
{
	void PropertiesPanel::OnImGuiRender()
	{
		if (!m_Visible)
			return;

		ImGui::Begin(m_Name.c_str(), &m_Visible);

		// Lock button in header
		bool hasSelection = Selection::HasSelection() || m_Locked;
		bool wasLocked = m_Locked;  // Save state before button click changes it

		if (wasLocked)
		{
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.4f, 0.1f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.5f, 0.2f, 1.0f));
		}

		if (ImGui::Button(m_Locked ? "Locked" : "Lock"))
		{
			if (m_Locked)
			{
				Unlock();
			}
			else if (Selection::HasSelection())
			{
				Lock();
			}
		}

		if (wasLocked)
		{
			ImGui::PopStyleColor(2);
		}

		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip(m_Locked ? "Click to unlock and follow selection" : "Lock to keep current view while browsing");
		}

		ImGui::SameLine();
		ImGui::Separator();

		// Get the selection to display (locked or current)
		Selectable primary;
		if (m_Locked)
		{
			primary = m_LockedSelection;
		}
		else if (Selection::HasSelection())
		{
			primary = Selection::GetPrimarySelection();
		}
		else
		{
			ImGui::TextDisabled("Nothing selected");
			ImGui::End();
			return;
		}

		// Validate locked selection still exists
		if (m_Locked && primary.Type == SelectableType::None)
		{
			Unlock();
			ImGui::TextDisabled("Locked item no longer exists");
			ImGui::End();
			return;
		}

		// Multi-selection info (only when not locked)
		if (!m_Locked && Selection::GetSelectionCount() > 1)
		{
			ImGui::Text("%zu items selected", Selection::GetSelectionCount());
			ImGui::Separator();
		}

		// Draw properties based on selection type
		switch (primary.Type)
		{
			case SelectableType::Asset:
				DrawAssetProperties(primary.ID);
				break;

			case SelectableType::Entity:
				EntityEditor::Draw(primary.ID);
				break;

			default:
				ImGui::TextDisabled("Unknown selection type");
				break;
		}

		ImGui::End();
	}

	Selectable PropertiesPanel::GetCurrentSelection() const
	{
		if (Selection::HasSelection())
			return Selection::GetPrimarySelection();
		return Selectable();
	}

	void PropertiesPanel::DrawAssetProperties(UUID assetUUID)
	{
		const AssetMetadata* metadata = AssetManager::GetRegistry().GetMetadata(assetUUID);

		if (!metadata)
		{
			ImGui::TextDisabled("Asset not found in registry");
			return;
		}

		// Draw type-specific editor
		switch (metadata->Type)
		{
			case AssetType::Texture2D:
				TextureEditor::Draw(assetUUID);
				break;

			case AssetType::Mesh:
				MeshEditor::Draw(assetUUID);
				break;

			case AssetType::Shader:
				ShaderEditor::Draw(assetUUID);
				break;

			case AssetType::Material:
				MaterialEditor::Draw(assetUUID);
				break;

			default:
				ImGui::Text("Asset Properties");
				ImGui::Separator();
				ImGui::Text("UUID: %llu", static_cast<uint64_t>(assetUUID));
				ImGui::Text("Type: %s", AssetTypeToString(metadata->Type));
				ImGui::Text("Path: %s", metadata->FilePath.string().c_str());
				ImGui::Separator();
				ImGui::TextDisabled("No editor available for this asset type");
				break;
		}
	}
}
