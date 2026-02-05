#pragma once

#include "imgui.h"
#include "CBEngine/Core/Core.h"
#include "CBEngine/Core/UUID.h"
#include "CBEngine/Renderer/Resources/Mesh.h"
#include "CBEngine/Asset/AssetMetadata.h"
#include "CBEngine/Asset/AssetManager.h"

namespace CB
{
	class MeshEditor
	{
	public:
		static void Draw(UUID meshUUID)
		{
			const AssetMetadata* metadata = AssetManager::GetRegistry().GetMetadata(meshUUID);
			if (!metadata || metadata->Type != AssetType::Mesh)
			{
				ImGui::TextDisabled("Invalid mesh");
				return;
			}

			Ref<Mesh> mesh = AssetManager::GetAsset<Mesh>(meshUUID);
			if (!mesh)
			{
				ImGui::TextDisabled("Failed to load mesh");
				return;
			}

			// Header
			ImGui::Text("Mesh: %s", metadata->FilePath.stem().string().c_str());
			ImGui::TextDisabled("Path: %s", metadata->FilePath.string().c_str());
			ImGui::Separator();

			// Statistics
			if (ImGui::CollapsingHeader("Statistics", ImGuiTreeNodeFlags_DefaultOpen))
			{
				uint32_t indexCount = mesh->GetIndexCount();
				uint32_t triangleCount = indexCount / 3;

				ImGui::Text("Index Count: %u", indexCount);
				ImGui::Text("Triangle Count: %u", triangleCount);

				// Estimate vertex count (rough estimate based on index count)
				// This is approximate since we don't have direct access to vertex count
				ImGui::TextDisabled("(Vertex data stored in GPU)");
			}

			// TODO: Add mesh preview viewport
			if (ImGui::CollapsingHeader("Preview"))
			{
				ImGui::TextDisabled("Mesh preview coming soon...");
				// Future: Add a small viewport to preview the mesh
			}
		}
	};
}
