#pragma once

#include "imgui.h"
#include "CBEngine/Scene/Entity.h"
#include "CBEngine/Components/VoxelAnchorComponent.h"
#include "../ComponentCard.h"

namespace CB
{
	class VoxelAnchorComponentView
	{
	public:
		static void Draw(Entity entity)
		{
			if (!entity.HasComponent<VoxelAnchorComponent>())
				return;

			bool removed = false;
			bool reset = false;

			bool opened = ComponentCard::Begin("Voxel Anchor", true, &removed, &reset);

			if (removed) {
				entity.RemoveComponent<VoxelAnchorComponent>();
				ComponentCard::End();
				return;
			}

			if (reset) { entity.GetComponent<VoxelAnchorComponent>() = VoxelAnchorComponent{}; }

			if (opened) {
				auto& anchor = entity.GetComponent<VoxelAnchorComponent>();

				ImGui::DragFloat("Radius", &anchor.Radius, 0.1f, 0.1f, 100.0f, "%.1f");
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("Anchor radius in world units — voxels within this distance are grounded");

				ImGui::Checkbox("Affects All", &anchor.AffectsAll);
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("If true, anchors all nearby destructible voxel entities");
			}

			ComponentCard::End();
		}
	};
}