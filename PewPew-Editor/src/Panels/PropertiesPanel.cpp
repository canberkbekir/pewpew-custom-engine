#include "PropertiesPanel.h"
#include "imgui.h"
#include "PewPew/Selection/Selection.h"
#include "PewPew/Asset/AssetManager.h"

namespace PewPew
{
	void PropertiesPanel::OnImGuiRender()
	{
		if (!m_Visible)
			return;

		ImGui::Begin(m_Name.c_str(), &m_Visible);

		if (!Selection::HasSelection())
		{
			ImGui::TextDisabled("Nothing selected");
			ImGui::End();
			return;
		}

		// Get primary selection
		Selectable primary = Selection::GetPrimarySelection();

		// Multi-selection info
		if (Selection::GetSelectionCount() > 1)
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
				DrawEntityProperties(primary.ID);
				break;

			default:
				ImGui::TextDisabled("Unknown selection type");
				break;
		}

		ImGui::End();
	}

	void PropertiesPanel::DrawAssetProperties(UUID assetUUID)
	{
		const AssetMetadata* metadata = AssetManager::GetRegistry().GetMetadata(assetUUID);

		if (!metadata)
		{
			ImGui::TextDisabled("Asset not found in registry");
			return;
		}

		// Show basic info
		ImGui::Text("Asset Properties");
		ImGui::Separator();

		ImGui::Text("UUID: %llu", static_cast<uint64_t>(assetUUID));
		ImGui::Text("Type: %s", AssetTypeToString(metadata->Type));
		ImGui::Text("Path: %s", metadata->FilePath.string().c_str());

		ImGui::Separator();

		// TODO: Add your custom property drawing here based on asset type
		// Example:
		// switch (metadata->Type)
		// {
		//     case AssetType::Texture2D:
		//         // Draw texture preview, import settings, etc.
		//         break;
		//     case AssetType::Mesh:
		//         // Draw mesh info, vertex count, etc.
		//         break;
		//     case AssetType::Shader:
		//         // Draw shader uniforms, compile status, etc.
		//         break;
		//     case AssetType::Material:
		//         // Draw material properties, textures, etc.
		//         break;
		// }

		ImGui::TextDisabled("(Add your property drawing code here)");
	}

	void PropertiesPanel::DrawEntityProperties(UUID entityUUID)
	{
		ImGui::Text("Entity Properties");
		ImGui::Separator();

		ImGui::Text("Entity UUID: %llu", static_cast<uint64_t>(entityUUID));

		ImGui::Separator();

		// TODO: Add your entity/component drawing here
		// When you have ECS, you can draw components like:
		// - Transform component
		// - Mesh component
		// - Material component
		// - Light component
		// - etc.

		ImGui::TextDisabled("(Add your component drawing code here)");
	}
}
