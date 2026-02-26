#pragma once

#include "imgui.h"
#include "CBEngine/Core/Core.h"
#include "CBEngine/Core/UUID.h"
#include "CBEngine/Renderer/Resources/Texture.h"
#include "CBEngine/Asset/AssetMetadata.h"
#include "CBEngine/Asset/AssetManager.h"

namespace CB
{
	class TextureEditor
	{
	public:
		static void Draw(UUID textureUUID)
		{
			const AssetMetadata* metadata = AssetManager::GetRegistry().GetMetadata(textureUUID);
			if (!metadata || metadata->Type != AssetType::Texture2D) {
				ImGui::TextDisabled("Invalid texture");
				return;
			}

			Ref<Texture2D> texture = AssetManager::GetAsset<Texture2D>(textureUUID);
			if (!texture) {
				ImGui::TextDisabled("Failed to load texture");
				return;
			}

			// Header
			ImGui::Text("Texture: %s", metadata->FilePath.stem().string().c_str());
			ImGui::TextDisabled("Path: %s", metadata->FilePath.string().c_str());
			ImGui::Separator();

			// Info
			ImGui::Text("Dimensions: %d x %d", texture->GetWidth(), texture->GetHeight());

			ImGui::Separator();

			// Preview
			ImGui::Text("Preview:");

			float maxPreviewSize = ImGui::GetContentRegionAvail().x;
			if (maxPreviewSize > 512.0f) maxPreviewSize = 512.0f;
			if (maxPreviewSize < 64.0f) maxPreviewSize = 64.0f;

			float aspect = static_cast<float>(texture->GetWidth()) / static_cast<float>(texture->GetHeight());
			ImVec2 previewSize;
			if (aspect > 1.0f) { previewSize = ImVec2(maxPreviewSize, maxPreviewSize / aspect); }
			else { previewSize = ImVec2(maxPreviewSize * aspect, maxPreviewSize); }

			ImGui::Image(
				reinterpret_cast<void*>(static_cast<uint64_t>(texture->GetRendererID())),
				previewSize,
				ImVec2(0, 1), ImVec2(1, 0) // Flip for OpenGL
			);
		}
	};
}