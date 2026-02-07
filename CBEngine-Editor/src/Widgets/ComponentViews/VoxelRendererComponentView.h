#pragma once

#include "imgui.h"
#include "CBEngine/Scene/Entity.h"
#include "CBEngine/Components/VoxelRendererComponent.h"
#include "CBEngine/Asset/AssetManager.h"
#include "CBEngine/Asset/ProcessedMeshAsset.h"
#include "CBEngine/Utils/VoxelizerAPI.h"
#include "CBEngine/Utils/VoxelMaterialType.h"
#include "CBEngine/Asset/VoxelTextureAsset.h"
#include "../AssetPicker.h"

namespace CB
{
	class VoxelRendererComponentView
	{
	public:
		static void Draw(Entity entity)
		{
			if (!entity.HasComponent<VoxelRendererComponent>())
				return;

			ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed
				| ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_SpanAvailWidth;

			bool opened = ImGui::TreeNodeEx("Voxel Renderer", flags);

			// Remove button
			ImGui::SameLine(ImGui::GetWindowWidth() - 25);
			if (ImGui::SmallButton("X##RemoveVoxelRenderer"))
			{
				entity.RemoveComponent<VoxelRendererComponent>();
				if (opened) ImGui::TreePop();
				return;
			}

			if (opened)
			{
				auto& voxelRenderer = entity.GetComponent<VoxelRendererComponent>();

				// Visible checkbox
				ImGui::Checkbox("Visible", &voxelRenderer.Visible);

				ImGui::Spacing();

				// VoxelMesh picker (.vmesh only)
				if (AssetPicker::DrawVoxelMesh("Voxel Mesh", voxelRenderer.VoxelMeshUUID))
				{
					voxelRenderer.MeshAsset = nullptr;
					voxelRenderer.HasPalette = false;
					voxelRenderer.PaletteColorTexture = nullptr;
					voxelRenderer.PaletteMaterialTexture = nullptr;
					if (voxelRenderer.VoxelMeshUUID.IsValid())
						voxelRenderer.ResolveAssets();
				}
				if (voxelRenderer.MeshAsset)
				{
					ImGui::SameLine();
					ImGui::TextDisabled("(%u tris)", voxelRenderer.MeshAsset->GetIndexCount() / 3);
				}

				// Color Texture picker (samples UVs to build voxel palette)
				if (AssetPicker::DrawTexture("Color Texture", voxelRenderer.ColorTextureUUID))
				{
					// Re-resolve to rebuild palette from new texture
					if (voxelRenderer.VoxelMeshUUID.IsValid())
						voxelRenderer.ResolveAssets();
				}

				// Material picker
				if (AssetPicker::DrawMaterial("Material", voxelRenderer.MaterialUUID))
				{
					if (voxelRenderer.MaterialUUID.IsValid())
						voxelRenderer.MaterialAsset = AssetManager::GetAsset<Material>(voxelRenderer.MaterialUUID);
					else
						voxelRenderer.MaterialAsset = nullptr;
				}
				if (voxelRenderer.MaterialAsset)
				{
					ImGui::SameLine();
					Vector3 albedo = voxelRenderer.MaterialAsset->GetAlbedo();
					ImGui::ColorButton("##MatPreview", ImVec4(albedo.x, albedo.y, albedo.z, 1.0f),
						ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoBorder, ImVec2(16, 16));
				}

				// Shader picker
				if (AssetPicker::DrawShader("Shader", voxelRenderer.ShaderUUID))
				{
					if (voxelRenderer.ShaderUUID.IsValid())
						voxelRenderer.ShaderAsset = AssetManager::GetAsset<Shader>(voxelRenderer.ShaderUUID);
					else
						voxelRenderer.ShaderAsset = nullptr;
				}

				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();

				// Voxel Texture picker (.vtex)
				if (AssetPicker::DrawVoxelTexture("Voxel Texture", voxelRenderer.VoxelTextureUUID))
				{
					if (voxelRenderer.VoxelTextureUUID.IsValid())
						voxelRenderer.VoxelTexture = AssetManager::GetAsset<VoxelTextureAsset>(voxelRenderer.VoxelTextureUUID);
					else
						voxelRenderer.VoxelTexture = nullptr;
				}

				// Show material type distribution if vtex is loaded
				if (voxelRenderer.VoxelTexture)
				{
					const auto& vtex = voxelRenderer.VoxelTexture;

					// Count material types from palette mapping
					uint32_t counts[(int)VoxelMaterialType::Count] = {};
					for (const auto& [idx, type] : vtex->PaletteMapping)
						counts[(int)type]++;

					ImGui::TextDisabled("Palette types:");
					for (int i = 0; i < (int)VoxelMaterialType::Count; i++)
					{
						if (counts[i] > 0)
						{
							VoxelMaterialType mt = static_cast<VoxelMaterialType>(i);
							Vector3 col = VoxelMaterialTypeDisplayColor(mt);
							ImGui::ColorButton(("##mt" + std::to_string(i)).c_str(),
								ImVec4(col.x, col.y, col.z, 1.0f),
								ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoBorder,
								ImVec2(12, 12));
							ImGui::SameLine();
							ImGui::Text("%s: %u", VoxelMaterialTypeToString(mt), counts[i]);
						}
					}

					if (!vtex->VoxelOverrides.empty())
						ImGui::TextDisabled("(%zu voxel overrides)", vtex->VoxelOverrides.size());

					// Regenerate button
					if (voxelRenderer.VoxelMeshUUID.IsValid())
					{
						if (ImGui::Button("Regenerate from VoxelMesh"))
						{
							auto vmesh = AssetManager::GetAsset<VoxelMeshAsset>(voxelRenderer.VoxelMeshUUID);
							if (vmesh)
							{
								auto newVtex = VoxelTextureAsset::GenerateFromVmesh(vmesh);
								if (newVtex)
								{
									voxelRenderer.VoxelTexture->PaletteMapping = newVtex->PaletteMapping;
									voxelRenderer.VoxelTexture->VoxelOverrides.clear();
									voxelRenderer.VoxelTexture->GridSize = newVtex->GridSize;
									voxelRenderer.VoxelTexture->VoxelCount = newVtex->VoxelCount;
								}
							}
						}
					}
				}

				// Show voxel info
				if (voxelRenderer.VoxelMeshUUID.IsValid())
				{
					ImGui::Spacing();
					ImGui::TextDisabled("Grid: %d | %s",
						voxelRenderer.VoxelSettings.GridSize,
						voxelRenderer.VoxelSettings.Solid ? "Solid" : "Surface");
				}

				ImGui::TreePop();
			}
		}
	};
}
