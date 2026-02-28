#include "SubstanceEditorPanel.h"
#include "CBEngine/Voxel/Substance/SubstanceRegistry.h"
#include "CBEngine/Voxel/Substance/SubstancePropertySchema.h"
#include "../Widgets/SubstanceEditorUtils.h"

#include <imgui.h>
#include <algorithm>
#include <cstring>

namespace CB
{
	void SubstanceEditorPanel::OnImGuiRender()
	{
		if (!m_Visible) return;

		ImGui::Begin("Substance Editor", &m_Visible);

		float panelWidth = ImGui::GetContentRegionAvail().x;
		float listWidth = glm::max(180.0f, panelWidth * 0.25f);

		// Left pane: substance list
		ImGui::BeginChild("##SubstanceList", ImVec2(listWidth, 0), true);
		DrawSubstanceList();
		ImGui::EndChild();

		ImGui::SameLine();

		// Right pane: inspector
		ImGui::BeginChild("##SubstanceInspector", ImVec2(0, 0), true);
		DrawSubstanceInspector();
		ImGui::EndChild();

		ImGui::End();
	}

	void SubstanceEditorPanel::DrawSubstanceList()
	{
		// Search filter
		ImGui::SetNextItemWidth(-1);
		ImGui::InputTextWithHint("##search", "Search...", m_SearchFilter, sizeof(m_SearchFilter));

		ImGui::Separator();

		// Substance list
		auto allIDs = SubstanceRegistry::GetAllIDs();
		std::string filterStr(m_SearchFilter);
		// Case-insensitive filter
		std::transform(filterStr.begin(), filterStr.end(), filterStr.begin(), ::tolower);

		for (const auto& id : allIDs) {
			if (!filterStr.empty()) {
				std::string lowerID = id;
				std::transform(lowerID.begin(), lowerID.end(), lowerID.begin(), ::tolower);
				if (lowerID.find(filterStr) == std::string::npos)
					continue;
			}

			Vector3 col = SubstanceRegistry::GetDisplayColor(id);
			ImGui::ColorButton(("##col_" + id).c_str(),
				ImVec4(col.x, col.y, col.z, 1.0f),
				ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoBorder,
				ImVec2(12, 12));
			ImGui::SameLine();

			bool selected = (m_SelectedID == id);
			if (ImGui::Selectable(id.c_str(), selected)) {
				m_SelectedID = id;
			}

			// Right-click context menu
			if (ImGui::BeginPopupContextItem(("##ctx_" + id).c_str())) {
				if (ImGui::MenuItem("Rename")) {
					std::strncpy(m_RenameBuffer, id.c_str(), sizeof(m_RenameBuffer) - 1);
					m_RenameBuffer[sizeof(m_RenameBuffer) - 1] = '\0';
					m_ShowRenamePopup = true;
					m_SelectedID = id;
				}
				if (ImGui::MenuItem("Duplicate")) {
					std::string newID = id + "_Copy";
					int suffix = 1;
					while (SubstanceRegistry::Exists(newID))
						newID = id + "_Copy" + std::to_string(++suffix);
					SubstanceRegistry::Duplicate(id, newID);
					m_SelectedID = newID;
				}
				if (ImGui::MenuItem("Delete")) {
					SubstanceRegistry::Remove(id);
					if (m_SelectedID == id)
						m_SelectedID.clear();
				}
				ImGui::EndPopup();
			}
		}

		ImGui::Separator();

		// Bottom buttons
		if (ImGui::Button("+ New")) {
			std::string newID = "NewSubstance";
			int suffix = 1;
			while (SubstanceRegistry::Exists(newID))
				newID = "NewSubstance" + std::to_string(++suffix);

			SubstanceDefinition def;
			def.ID = newID;
			SubstanceRegistry::Register(std::move(def));
			m_SelectedID = newID;
		}
		ImGui::SameLine();
		if (ImGui::Button("Duplicate") && !m_SelectedID.empty()) {
			std::string newID = m_SelectedID + "_Copy";
			int suffix = 1;
			while (SubstanceRegistry::Exists(newID))
				newID = m_SelectedID + "_Copy" + std::to_string(++suffix);
			SubstanceRegistry::Duplicate(m_SelectedID, newID);
			m_SelectedID = newID;
		}

		// Rename popup
		if (m_ShowRenamePopup) {
			ImGui::OpenPopup("Rename Substance");
			m_ShowRenamePopup = false;
		}
		if (ImGui::BeginPopup("Rename Substance")) {
			ImGui::Text("Rename: %s", m_SelectedID.c_str());
			ImGui::InputText("New Name", m_RenameBuffer, sizeof(m_RenameBuffer));
			if (ImGui::Button("OK")) {
				std::string newID(m_RenameBuffer);
				if (!newID.empty() && newID != m_SelectedID && !SubstanceRegistry::Exists(newID)) {
					SubstanceRegistry::Rename(m_SelectedID, newID);
					m_SelectedID = newID;
				}
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel"))
				ImGui::CloseCurrentPopup();
			ImGui::EndPopup();
		}
	}

	void SubstanceEditorPanel::DrawSubstanceInspector()
	{
		if (m_SelectedID.empty() || !SubstanceRegistry::Exists(m_SelectedID)) {
			ImGui::TextDisabled("Select a substance from the list");
			return;
		}

		auto& def = SubstanceRegistry::GetDefinition(m_SelectedID);
		auto& props = def.Properties;

		// Header: name + display color + PBR
		ImGui::Text("%s", def.ID.c_str());
		ImGui::Separator();

		float col[3] = {def.DisplayColor.x, def.DisplayColor.y, def.DisplayColor.z};
		if (ImGui::ColorEdit3("Display Color", col))
			def.DisplayColor = Vector3(col[0], col[1], col[2]);

		if (ImGui::CollapsingHeader("PBR Defaults")) {
			ImGui::DragFloat("Metallic", &def.PBRDefaults.Metallic, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat("Roughness", &def.PBRDefaults.Roughness, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat("Emission", &def.PBRDefaults.Emission, 0.01f, 0.0f, 10.0f);
		}

		ImGui::Separator();

		// Schema-driven property categories
		DrawSubstanceCategory("Physical", props, true);
		DrawSubstanceCategory("Structural", props, true);
		DrawSubstanceCategory("Fracture", props);

		// Environmental section — needs special handling for burn stages
		{
			ImGuiTreeNodeFlags flags = 0;
			if (ImGui::CollapsingHeader("Environmental", flags)) {
				const auto& schema = GetSubstancePropertySchema();
				for (const auto& meta : schema) {
					if (std::string(meta.Category) != "Environmental")
						continue;
					if (meta.Widget == SubstancePropWidget::Custom) {
						// Burn stages custom widget
						if (std::string(meta.YAMLKey) == "burn_stages") {
							if (IsPropertyEnabled(meta, props)) {
								ImGui::Separator();
								DrawBurnStages(props, def.DisplayColor);
							}
						}
						continue;
					}
					if (!IsPropertyEnabled(meta, props))
						continue;

					ImGui::PushID(meta.YAMLKey);
					DrawSubstanceProperty(meta, props);
					ImGui::PopID();
				}
			}
		}

		// Damage Tints section
		if (ImGui::CollapsingHeader("Damage Tints"))
			DrawDamageTints(props);

		ImGui::Separator();

		// Footer: Save / Save All / Reload
		if (ImGui::Button("Save All"))
			SubstanceRegistry::Save("assets/config/voxel_substances.yaml");
		ImGui::SameLine();
		if (ImGui::Button("Reload"))
			SubstanceRegistry::Load("assets/config/voxel_substances.yaml");
	}
}
