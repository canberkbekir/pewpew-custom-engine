#pragma once

#include "imgui.h"
#include "CBEngine/Scene/Entity.h"
#include "CBEngine/Components/VoxelRendererComponent.h"
#include "CBEngine/Asset/AssetManager.h"
#include "CBEngine/Asset/ProcessedMeshAsset.h"
#include "CBEngine/Utils/VoxelizerAPI.h"
#include "CBEngine/Utils/VoxelMaterialType.h"
#include "CBEngine/Asset/VoxelTextureAsset.h"
#include "CBEngine/Voxel/Destruction/SubstanceRegistry.h"
#include "../AssetPicker.h"
#include "../ComponentCard.h"

namespace CB
{
	class VoxelRendererComponentView
	{
	public:
		static void Draw(Entity entity)
		{
			if (!entity.HasComponent<VoxelRendererComponent>())
				return;

			bool removed = false;
			bool reset = false;
			bool regenerate = false;

			std::vector<ComponentCard::MenuAction> extraActions;
			extraActions.push_back({"Regenerate from VoxelMesh", &regenerate});

			bool opened = ComponentCard::Begin("Voxel Renderer", true, &removed, &reset, extraActions);

			if (removed) {
				entity.RemoveComponent<VoxelRendererComponent>();
				ComponentCard::End();
				return;
			}

			if (reset) {
				auto& vr = entity.GetComponent<VoxelRendererComponent>();
				vr.VoxelMeshUUID = UUID();
				vr.VoxelTextureUUID = UUID();
				vr.MeshAsset = nullptr;
				vr.VoxelTexture = nullptr;
				vr.HasPalette = false;
				vr.PaletteColorTexture = nullptr;
				vr.PaletteMaterialTexture = nullptr;
				vr.Visible = true;
			}

			auto& voxelRenderer = entity.GetComponent<VoxelRendererComponent>();

			if (regenerate && voxelRenderer.VoxelMeshUUID.IsValid() && voxelRenderer.VoxelTexture) {
				auto vmesh = AssetManager::GetAsset<VoxelMeshAsset>(voxelRenderer.VoxelMeshUUID);
				if (vmesh) {
					auto newVtex = VoxelTextureAsset::GenerateFromVmesh(vmesh);
					if (newVtex) {
						voxelRenderer.VoxelTexture->PaletteMapping = newVtex->PaletteMapping;
						voxelRenderer.VoxelTexture->VoxelOverrides.clear();
						voxelRenderer.VoxelTexture->GridSize = newVtex->GridSize;
						voxelRenderer.VoxelTexture->VoxelCount = newVtex->VoxelCount;
					}
				}
			}

			if (opened) {
				// Visible checkbox
				ImGui::Checkbox("Visible", &voxelRenderer.Visible);

				ImGui::Spacing();

				// VoxelMesh picker (.vmesh only)
				if (AssetPicker::DrawVoxelMesh("Voxel Mesh", voxelRenderer.VoxelMeshUUID)) {
					voxelRenderer.MeshAsset = nullptr;
					voxelRenderer.HasPalette = false;
					voxelRenderer.PaletteColorTexture = nullptr;
					voxelRenderer.PaletteMaterialTexture = nullptr;
					if (voxelRenderer.VoxelMeshUUID.IsValid())
						voxelRenderer.ResolveAssets();
				}
				if (voxelRenderer.MeshAsset) {
					ImGui::SameLine();
					ImGui::TextDisabled("(%u tris)", voxelRenderer.MeshAsset->GetIndexCount() / 3);
				}

				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();

				// Voxel Texture picker (.vtex)
				if (AssetPicker::DrawVoxelTexture("Voxel Texture", voxelRenderer.VoxelTextureUUID)) {
					if (voxelRenderer.VoxelTextureUUID.IsValid()) {
						voxelRenderer.VoxelTexture = AssetManager::GetAsset<VoxelTextureAsset>(
							voxelRenderer.VoxelTextureUUID);
						if (voxelRenderer.VoxelMeshUUID.IsValid())
							voxelRenderer.ResolveAssets();
					}
					else {
						voxelRenderer.VoxelTexture = nullptr;
						if (voxelRenderer.VoxelMeshUUID.IsValid())
							voxelRenderer.ResolveAssets();
					}
				}

				// Show substance distribution if vtex is loaded
				if (voxelRenderer.VoxelTexture) {
					const auto& vtex = voxelRenderer.VoxelTexture;

					// Count substances from palette mapping
					std::unordered_map<SubstanceID, uint32_t> subCounts;
					for (const auto& [idx, id] : vtex->PaletteMapping)
						subCounts[id]++;

					ImGui::TextDisabled("Palette substances:");
					for (const auto& [id, count] : subCounts) {
						Vector3 col = SubstanceRegistry::GetDisplayColor(id);
						ImGui::ColorButton(("##mt" + id).c_str(),
						                   ImVec4(col.x, col.y, col.z, 1.0f),
						                   ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoBorder,
						                   ImVec2(12, 12));
						ImGui::SameLine();
						ImGui::Text("%s: %u", id.c_str(), count);
					}

					if (!vtex->VoxelOverrides.empty())
						ImGui::TextDisabled("(%zu voxel overrides)", vtex->VoxelOverrides.size());

					// PBR override indicators
					if (vtex->HasMetallicOverrides || vtex->HasRoughnessOverrides
						|| vtex->HasEmissionOverrides || vtex->HasAlbedoOverrides) {
						ImGui::TextDisabled("PBR overrides:");
						if (vtex->HasMetallicOverrides) {
							ImGui::SameLine();
							ImGui::Text("Met");
						}
						if (vtex->HasRoughnessOverrides) {
							ImGui::SameLine();
							ImGui::Text("Rough");
						}
						if (vtex->HasEmissionOverrides) {
							ImGui::SameLine();
							ImGui::Text("Emis");
						}
						if (vtex->HasAlbedoOverrides) {
							ImGui::SameLine();
							ImGui::Text("Albedo");
						}
					}
				}
			}

			ComponentCard::End();
		}
	};
}