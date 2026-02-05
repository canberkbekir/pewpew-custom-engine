#pragma once

#include "imgui.h"
#include "CBEngine/Core/Core.h"
#include "CBEngine/Core/UUID.h"
#include "CBEngine/Renderer/Resources/Shader.h"
#include "CBEngine/Asset/AssetMetadata.h"
#include "CBEngine/Asset/AssetManager.h"

namespace CB
{
	class ShaderEditor
	{
	public:
		static void Draw(UUID shaderUUID)
		{
			const AssetMetadata* metadata = AssetManager::GetRegistry().GetMetadata(shaderUUID);
			if (!metadata || metadata->Type != AssetType::Shader)
			{
				ImGui::TextDisabled("Invalid shader");
				return;
			}

			Ref<Shader> shader = AssetManager::GetAsset<Shader>(shaderUUID);
			if (!shader)
			{
				ImGui::TextDisabled("Failed to load shader");
				return;
			}

			// Header
			ImGui::Text("Shader: %s", shader->GetName().c_str());
			ImGui::TextDisabled("Path: %s", metadata->FilePath.string().c_str());
			ImGui::Separator();

			// Info
			if (ImGui::CollapsingHeader("Shader Info", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::Text("Name: %s", shader->GetName().c_str());
				ImGui::TextDisabled("Compiled and ready to use");
			}

			// Reload button
			ImGui::Separator();
			if (ImGui::Button("Reload Shader"))
			{
				AssetManager::QueueReload(shaderUUID);
			}
			ImGui::SameLine();
			ImGui::TextDisabled("Hot-reload support");
		}
	};
}
