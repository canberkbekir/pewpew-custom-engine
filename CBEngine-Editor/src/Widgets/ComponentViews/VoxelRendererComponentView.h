#pragma once

#include "imgui.h"
#include "CBEngine/Scene/Entity.h"
#include "CBEngine/Components/VoxelRendererComponent.h"
#include "CBEngine/Asset/AssetManager.h"
#include "CBEngine/Asset/ProcessedMeshAsset.h"
#include "CBEngine/Utils/VoxelizerAPI.h"
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
					if (voxelRenderer.VoxelMeshUUID.IsValid())
					{
						auto vmAsset = AssetManager::GetAsset<VoxelMeshAsset>(voxelRenderer.VoxelMeshUUID);
						if (vmAsset && vmAsset->VoxelCount > 0)
						{
							voxelRenderer.MeshAsset = VoxelizerAPI::CreateMeshFromGrid(vmAsset->GridData);
							voxelRenderer.VoxelSettings = vmAsset->VoxelSettings;
						}
					}
				}
				if (voxelRenderer.MeshAsset)
				{
					ImGui::SameLine();
					ImGui::TextDisabled("(%u tris)", voxelRenderer.MeshAsset->GetIndexCount() / 3);
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
