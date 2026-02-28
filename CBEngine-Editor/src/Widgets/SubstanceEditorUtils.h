#pragma once

#include "CBEngine/Voxel/Substance/SubstancePropertyMeta.h"
#include "CBEngine/Voxel/Substance/SubstancePropertySchema.h"
#include "CBEngine/Voxel/Substance/VoxelSubstance.h"

#include <imgui.h>
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>

namespace CB
{
	// =========================================================================
	// Schema-driven property drawing
	// =========================================================================

	// Returns true if property value was changed
	inline bool DrawSubstanceProperty(const SubstancePropertyMeta& meta, VoxelSubstanceProperties& props)
	{
		char* base = reinterpret_cast<char*>(&props);
		bool changed = false;

		switch (meta.Widget) {
		case SubstancePropWidget::DragFloat:
		{
			float* ptr = reinterpret_cast<float*>(base + meta.Offset);
			changed = ImGui::DragFloat(meta.DisplayName, ptr, meta.Speed, meta.Min, meta.Max);
			break;
		}
		case SubstancePropWidget::SliderFloat:
		{
			float* ptr = reinterpret_cast<float*>(base + meta.Offset);
			changed = ImGui::SliderFloat(meta.DisplayName, ptr, meta.Min, meta.Max);
			break;
		}
		case SubstancePropWidget::SliderInt:
		{
			int* ptr = reinterpret_cast<int*>(base + meta.Offset);
			changed = ImGui::SliderInt(meta.DisplayName, ptr, static_cast<int>(meta.Min), static_cast<int>(meta.Max));
			break;
		}
		case SubstancePropWidget::DragInt:
		{
			int* ptr = reinterpret_cast<int*>(base + meta.Offset);
			changed = ImGui::DragInt(meta.DisplayName, ptr, meta.Speed, static_cast<int>(meta.Min), static_cast<int>(meta.Max));
			break;
		}
		case SubstancePropWidget::Checkbox:
		{
			bool* ptr = reinterpret_cast<bool*>(base + meta.Offset);
			changed = ImGui::Checkbox(meta.DisplayName, ptr);
			break;
		}
		case SubstancePropWidget::Combo:
		{
			int* ptr = reinterpret_cast<int*>(base + meta.Offset);
			if (meta.EnumNames && meta.EnumCount > 0)
				changed = ImGui::Combo(meta.DisplayName, ptr, meta.EnumNames, meta.EnumCount);
			break;
		}
		case SubstancePropWidget::ColorEdit3:
		{
			Vector3* ptr = reinterpret_cast<Vector3*>(base + meta.Offset);
			float col[3] = {ptr->x, ptr->y, ptr->z};
			if (ImGui::ColorEdit3(meta.DisplayName, col)) {
				*ptr = Vector3(col[0], col[1], col[2]);
				changed = true;
			}
			break;
		}
		case SubstancePropWidget::Custom:
			break; // handled separately
		}

		if (meta.Tooltip && ImGui::IsItemHovered())
			ImGui::SetTooltip("%s", meta.Tooltip);

		return changed;
	}

	// Check if a conditional field's enabling bool is true
	inline bool IsPropertyEnabled(const SubstancePropertyMeta& meta, const VoxelSubstanceProperties& props)
	{
		if (!meta.EnabledBy)
			return true;

		// Look up the EnabledBy field in the schema
		const auto& schema = GetSubstancePropertySchema();
		for (const auto& other : schema) {
			if (other.Type == SubstancePropType::Bool &&
				std::string(other.YAMLKey) == meta.EnabledBy) {
				const bool* ptr = reinterpret_cast<const bool*>(
					reinterpret_cast<const char*>(&props) + other.Offset);
				return *ptr;
			}
		}

		// Try matching by field name directly (e.g. "Flammable" matches yaml key "flammable")
		// Use case-insensitive comparison for the first char
		for (const auto& other : schema) {
			if (other.Type != SubstancePropType::Bool) continue;
			std::string displayName = other.DisplayName;
			if (displayName == meta.EnabledBy)
			{
				const bool* ptr = reinterpret_cast<const bool*>(
					reinterpret_cast<const char*>(&props) + other.Offset);
				return *ptr;
			}
		}

		return true; // default: show
	}

	// Draw all properties in a category
	inline void DrawSubstanceCategory(const char* category, VoxelSubstanceProperties& props,
		bool defaultOpen = false)
	{
		ImGuiTreeNodeFlags flags = defaultOpen ? ImGuiTreeNodeFlags_DefaultOpen : 0;
		if (!ImGui::CollapsingHeader(category, flags))
			return;

		const auto& schema = GetSubstancePropertySchema();
		for (const auto& meta : schema) {
			if (std::string(meta.Category) != category)
				continue;
			if (meta.Widget == SubstancePropWidget::Custom)
				continue; // handled separately
			if (!IsPropertyEnabled(meta, props))
				continue;

			ImGui::PushID(meta.YAMLKey);
			DrawSubstanceProperty(meta, props);
			ImGui::PopID();
		}
	}

	// =========================================================================
	// Damage Tints (custom widget)
	// =========================================================================

	inline void DrawDamageTints(VoxelSubstanceProperties& props)
	{
		static const char* damageTypeNames[] = {
			"Impact", "Explosion", "Slice", "Fire", "Acid", "Pressure", "Structural"
		};
		static constexpr VoxelDamageType damageTypeValues[] = {
			VoxelDamageType::Impact, VoxelDamageType::Explosion, VoxelDamageType::Slice,
			VoxelDamageType::Fire, VoxelDamageType::Acid, VoxelDamageType::Pressure,
			VoxelDamageType::Structural
		};

		std::vector<VoxelDamageType> toRemove;
		for (auto& [dmgType, cfg] : props.DamageTints) {
			auto typeName = "Unknown";
			for (int i = 0; i < 7; ++i)
				if (damageTypeValues[i] == dmgType) { typeName = damageTypeNames[i]; break; }

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

		ImGui::Separator();
		static int addTypeIdx = 3;
		ImGui::Combo("##AddType", &addTypeIdx, damageTypeNames, 7);
		ImGui::SameLine();
		if (ImGui::Button("Add Tint")) {
			VoxelDamageType addType = damageTypeValues[addTypeIdx];
			if (props.DamageTints.find(addType) == props.DamageTints.end())
				props.DamageTints[addType] = DamageTintConfig{};
		}
	}

	// =========================================================================
	// Burn Stage Tints (custom widget with preview strip)
	// =========================================================================

	inline void DrawBurnStages(VoxelSubstanceProperties& props, const Vector3& sampleColor)
	{
		static const char* stageNames[] = {"Normal", "Burning", "Charred", "Collapse"};

		ImGui::Text("Burn Stage Tints:");
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Tint multipliers applied to voxel's original color.\n"
				"(1,1,1) = no tint, (0.1,0.08,0.05) = very dark.");

		for (int i = 0; i < VoxelSubstanceProperties::kBurnStageCount; ++i) {
			ImGui::PushID(i);
			float col[3] = {props.BurnStageTints[i].x, props.BurnStageTints[i].y, props.BurnStageTints[i].z};
			char label[64];
			snprintf(label, sizeof(label), "%s##stage", stageNames[i]);
			if (ImGui::ColorEdit3(label, col))
				props.BurnStageTints[i] = Vector3(col[0], col[1], col[2]);

			if (i > 0) {
				ImGui::SameLine();
				ImGui::Text("at %.0f%%", props.BurnStageThresholds[i - 1] * 100.0f);
			}
			else {
				ImGui::SameLine();
				ImGui::Text("at 0%%");
			}
			ImGui::PopID();
		}

		ImGui::Text("Thresholds:");
		for (int i = 0; i < 3; ++i) {
			ImGui::PushID(100 + i);
			char label[64];
			snprintf(label, sizeof(label), "%s -> %s", stageNames[i], stageNames[i + 1]);
			ImGui::SliderFloat(label, &props.BurnStageThresholds[i], 0.0f, 1.0f, "%.2f");
			ImGui::PopID();
		}

		// Enforce ordering
		for (int i = 1; i < 3; ++i)
			props.BurnStageThresholds[i] = glm::max(props.BurnStageThresholds[i],
				props.BurnStageThresholds[i - 1] + 0.01f);

		// Preview strip
		ImGui::Text("Preview on sample color:");
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		ImVec2 pos = ImGui::GetCursorScreenPos();
		constexpr int kStripCount = 10;
		constexpr float kCellSize = 24.0f;

		for (int s = 0; s < kStripCount; ++s) {
			float progress = static_cast<float>(s) / static_cast<float>(kStripCount - 1);

			// Determine burn stage + lerp
			int stageIdx = 0;
			for (int t = 2; t >= 0; --t) {
				if (progress >= props.BurnStageThresholds[t]) { stageIdx = t + 1; break; }
			}
			float stageStart = (stageIdx > 0) ? props.BurnStageThresholds[stageIdx - 1] : 0.0f;
			float stageEnd = (stageIdx < 3) ? props.BurnStageThresholds[stageIdx] : 1.0f;
			float t = glm::clamp(
				(progress - stageStart) / glm::max(stageEnd - stageStart, 0.001f), 0.0f, 1.0f);
			int nextStage = glm::min(stageIdx + 1, 3);
			Vector3 tint = glm::mix(props.BurnStageTints[stageIdx], props.BurnStageTints[nextStage], t);

			// Apply tint as multiplier to sample color
			Vector3 finalColor = sampleColor * tint;
			finalColor = glm::clamp(finalColor, Vector3(0.0f), Vector3(1.0f));

			ImVec2 pMin(pos.x + s * (kCellSize + 2), pos.y);
			ImVec2 pMax(pMin.x + kCellSize, pMin.y + kCellSize);
			drawList->AddRectFilled(pMin, pMax,
				IM_COL32(static_cast<int>(finalColor.r * 255),
				         static_cast<int>(finalColor.g * 255),
				         static_cast<int>(finalColor.b * 255), 255));
			drawList->AddRect(pMin, pMax, IM_COL32(128, 128, 128, 128));
		}

		// Advance cursor past the strip
		ImGui::Dummy(ImVec2(kStripCount * (kCellSize + 2), kCellSize + 4));

		// Label strip
		ImGui::Text("0%%");
		for (int s = 1; s < kStripCount; ++s) {
			ImGui::SameLine();
			float progress = static_cast<float>(s) / static_cast<float>(kStripCount - 1);
			ImGui::Text("%.0f%%", progress * 100.0f);
		}
	}
}
