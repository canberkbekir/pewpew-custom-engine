#include "SubstanceEditorPanel.h"
#include "CBEngine/Voxel/Destruction/VoxelSubstanceDatabase.h"

#include <imgui.h>

namespace CB
{
	void SubstanceEditorPanel::OnImGuiRender()
	{
		if (!m_Visible) return;

		ImGui::Begin("Substance Editor", &m_Visible);

		// Substance selector (tab bar with one tab per material type)
		if (ImGui::BeginTabBar("Substances")) {
			for (int i = 0; i < static_cast<int>(VoxelMaterialType::Count); ++i) {
				auto type = static_cast<VoxelMaterialType>(i);
				const char* name = VoxelMaterialTypeToString(type);
				if (ImGui::BeginTabItem(name)) {
					m_SelectedSubstance = i;
					DrawSubstance(type);
					ImGui::EndTabItem();
				}
			}
			ImGui::EndTabBar();
		}

		ImGui::Separator();

		// Save/Reload buttons
		if (ImGui::Button("Save to YAML"))
			VoxelSubstanceDatabase::Save("assets/config/voxel_substances.yaml");
		ImGui::SameLine();
		if (ImGui::Button("Reload from YAML"))
			VoxelSubstanceDatabase::Load("assets/config/voxel_substances.yaml");

		ImGui::End();
	}

	void SubstanceEditorPanel::DrawSubstance(VoxelMaterialType type)
	{
		auto& p = VoxelSubstanceDatabase::GetMutable(type);

		// --- Physical ---
		if (ImGui::CollapsingHeader("Physical", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::DragFloat("Mass Per Voxel", &p.MassPerVoxel, 0.001f, 0.001f, 10.0f, "%.4f");
		}

		// --- Structural ---
		if (ImGui::CollapsingHeader("Structural", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::DragFloat("Health", &p.Health, 1.0f, 1.0f, 10000.0f);
			ImGui::SliderFloat("Hardness", &p.Hardness, 0.0f, 100.0f);
			ImGui::SliderFloat("Explosion Resist", &p.ExplosionResistance, 0.0f, 1.0f);
			ImGui::SliderFloat("Slice Resist", &p.SliceResistance, 0.0f, 1.0f);
			ImGui::DragFloat("Tensile Strength", &p.TensileStrength, 1.0f, 0.0f, 100000.0f);
			ImGui::DragFloat("Impact Threshold", &p.ImpactThreshold, 0.1f, 0.0f, 1000.0f);
		}

		// --- Fracture ---
		if (ImGui::CollapsingHeader("Fracture")) {
			const char* fractureNames[] = {"None", "Chip", "Crack", "Shatter", "Crumble"};
			int fracIdx = static_cast<int>(p.Fracture);
			if (ImGui::Combo("Behavior", &fracIdx, fractureNames, 5))
				p.Fracture = static_cast<FractureBehavior>(fracIdx);
			ImGui::SliderFloat("Fracture Threshold", &p.FractureThreshold, 0.0f, 1.0f);
			ImGui::SliderInt("Fragment Count", &p.FragmentCount, 0, 20);
			ImGui::Checkbox("Fragments Have Physics", &p.FragmentsHavePhysics);
		}

		// --- Environmental ---
		if (ImGui::CollapsingHeader("Environmental")) {
			ImGui::Checkbox("Flammable", &p.Flammable);
			if (p.Flammable) {
				ImGui::DragFloat("Ignition Temperature", &p.IgnitionTemperature, 1.0f, 0.0f, 1000.0f);
				ImGui::DragFloat("Burn Duration (s)", &p.BurnDuration, 0.1f, 0.0f, 60.0f);
			}
			ImGui::Checkbox("Propagates Damage", &p.PropagatesDamage);
			if (p.PropagatesDamage)
				ImGui::SliderFloat("Spread Factor", &p.DamageSpreadFactor, 0.0f, 1.0f);
		}

		// --- Damage Tints ---
		DrawDamageTints(p);
	}

	void SubstanceEditorPanel::DrawDamageTints(VoxelSubstanceProperties& props)
	{
		if (!ImGui::CollapsingHeader("Damage Tints"))
			return;

		static const char* damageTypeNames[] = {
			"Impact", "Explosion", "Slice", "Fire", "Acid", "Pressure", "Structural"
		};
		static constexpr VoxelDamageType damageTypeValues[] = {
			VoxelDamageType::Impact, VoxelDamageType::Explosion, VoxelDamageType::Slice,
			VoxelDamageType::Fire, VoxelDamageType::Acid, VoxelDamageType::Pressure,
			VoxelDamageType::Structural
		};

		// Iterate configured tints
		std::vector<VoxelDamageType> toRemove;
		for (auto& [dmgType, cfg] : props.DamageTints) {
			auto typeName = "Unknown";
			for (int i = 0; i < 7; ++i)
				if (damageTypeValues[i] == dmgType) {
					typeName = damageTypeNames[i];
					break;
				}

			ImGui::PushID(static_cast<int>(dmgType));
			if (ImGui::TreeNode(typeName)) {
				float col[3] = {cfg.Color.x, cfg.Color.y, cfg.Color.z};
				if (ImGui::ColorEdit3("Tint Color", col))
					cfg.Color = Vector3(col[0], col[1], col[2]);
				ImGui::DragFloat("Intensity", &cfg.Intensity, 0.01f, 0.0f, 5.0f);
				ImGui::SliderInt("Spread Radius", &cfg.SpreadRadius, 0, 4);
				ImGui::SliderFloat("Spread Falloff", &cfg.SpreadFalloff, 0.0f, 1.0f);

				if (ImGui::Button("Remove"))
					toRemove.push_back(dmgType);

				ImGui::TreePop();
			}
			ImGui::PopID();
		}

		for (auto t : toRemove)
			props.DamageTints.erase(t);

		// Add new tint config
		ImGui::Separator();
		static int addTypeIdx = 3; // default to Fire
		ImGui::Combo("##AddType", &addTypeIdx, damageTypeNames, 7);
		ImGui::SameLine();
		if (ImGui::Button("Add Tint")) {
			VoxelDamageType addType = damageTypeValues[addTypeIdx];
			if (props.DamageTints.find(addType) == props.DamageTints.end())
				props.DamageTints[addType] = DamageTintConfig{};
		}
	}
}