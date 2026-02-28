#pragma once

#include "imgui.h"
#include "CBEngine/Scene/Entity.h"
#include "CBEngine/Components/DestructibleVoxelComponent.h"
#include "CBEngine/Voxel/Substance/SubstanceRegistry.h"
#include "../ComponentCard.h"

namespace CB
{
	class DestructibleVoxelComponentView
	{
	public:
		static void Draw(Entity entity)
		{
			if (!entity.HasComponent<DestructibleVoxelComponent>())
				return;

			bool removed = false;
			bool reset = false;

			bool opened = ComponentCard::Begin("Destructible Voxel", true, &removed, &reset);

			if (removed) {
				entity.RemoveComponent<DestructibleVoxelComponent>();
				ComponentCard::End();
				return;
			}

			if (reset) { entity.GetComponent<DestructibleVoxelComponent>() = DestructibleVoxelComponent{}; }

			if (opened) {
				auto& dv = entity.GetComponent<DestructibleVoxelComponent>();

				ImGui::Checkbox("Enabled", &dv.Enabled);

				// --- Physics Auto-Impact ---
				ImGui::Spacing();
				ImGui::TextUnformatted("Physics Impact");
				ImGui::Separator();

				ImGui::Checkbox("Receive Physics Impact", &dv.ReceivePhysicsImpact);
				if (dv.ReceivePhysicsImpact) {
					ImGui::DragFloat("Impact Threshold", &dv.PhysicsImpactThreshold, 0.5f, 0.0f, 1000.0f, "%.1f");
					if (ImGui::IsItemHovered())
						ImGui::SetTooltip("Minimum impact speed before damage registers");

					ImGui::DragFloat("Impact Multiplier", &dv.PhysicsImpactMultiplier, 0.05f, 0.0f, 10.0f, "%.2f");
					if (ImGui::IsItemHovered())
						ImGui::SetTooltip("Scale applied to physics impact damage");
				}

				// --- Structural Integrity ---
				ImGui::Spacing();
				ImGui::TextUnformatted("Structural Integrity");
				ImGui::Separator();

				ImGui::Checkbox("Structural Integrity", &dv.StructuralIntegrityEnabled);
				if (dv.StructuralIntegrityEnabled) {
					const char* anchorModeNames[] = {"Bottom Face", "Manual", "None"};
					int currentAnchor = static_cast<int>(dv.Anchor);
					if (ImGui::Combo("Anchor Mode", &currentAnchor, anchorModeNames, IM_ARRAYSIZE(anchorModeNames)))
						dv.Anchor = static_cast<DestructibleVoxelComponent::AnchorMode>(currentAnchor);

					if (ImGui::IsItemHovered()) {
						ImGui::SetTooltip(
							"BottomFace: bottom voxel row is anchored to ground.\n"
							"  If all bottom voxels are destroyed, structure collapses.\n"
							"Manual: only VoxelAnchor entities count as anchors.\n"
							"None: no collapse simulation (decorative rubble, etc.)");
					}
				}

				// --- Substance ---
				ImGui::Spacing();
				ImGui::TextUnformatted("Substance");
				ImGui::Separator();

				// Substance picker combo
				{
					auto allIDs = SubstanceRegistry::GetAllIDs();
					// Prepend empty option for "per-voxel lookup"
					int currentIdx = 0;
					std::vector<std::string> options;
					options.push_back("(Per-Voxel)");
					for (const auto& id : allIDs) {
						options.push_back(id);
						if (id == dv.SubstanceOverride)
							currentIdx = static_cast<int>(options.size()) - 1;
					}
					std::vector<const char*> optionPtrs;
					for (const auto& o : options)
						optionPtrs.push_back(o.c_str());

					if (ImGui::Combo("Substance Override", &currentIdx,
						optionPtrs.data(), static_cast<int>(optionPtrs.size()))) {
						dv.SubstanceOverride = (currentIdx == 0) ? "" : options[currentIdx];
					}
					if (ImGui::IsItemHovered())
						ImGui::SetTooltip("Per-Voxel = uses VoxelTextureAsset mapping.\nOtherwise forces all voxels to selected substance.");
				}

				ImGui::DragFloat("Health Multiplier", &dv.HealthMultiplier, 0.05f, 0.01f, 100.0f, "%.2f");
				ImGui::DragFloat("Resistance Multiplier", &dv.ResistanceMultiplier, 0.05f, 0.01f, 100.0f, "%.2f");

				// --- Performance Budget ---
				ImGui::Spacing();
				ImGui::TextUnformatted("Performance");
				ImGui::Separator();

				ImGui::DragFloat("Fragment Spawn Distance", &dv.FragmentSpawnDistance, 1.0f, 1.0f, 500.0f, "%.0f");
				ImGui::DragFloat("Structural Check Distance", &dv.StructuralCheckDistance, 1.0f, 1.0f, 500.0f, "%.0f");
				ImGui::DragInt("Max Fragments", &dv.MaxFragmentsPerObject, 1, 1, 100);
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("Maximum fragment entities spawned per destruction event.\n"
						"Smallest pieces are discarded when limit is exceeded.\n"
						"Lower = better performance, Higher = more debris detail.");

				ImGui::DragInt("Max Destructions/Frame", &dv.MaxDestructionsPerFrame, 1, 1, 200);
			}

			ComponentCard::End();
		}
	};
}