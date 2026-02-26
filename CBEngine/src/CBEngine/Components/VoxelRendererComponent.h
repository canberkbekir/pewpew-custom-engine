#pragma once

#include "CBEngine/Core/Core.h"
#include "CBEngine/Core/UUID.h"
#include "CBEngine/Renderer/Resources/Mesh.h"
#include "CBEngine/Renderer/Resources/Texture.h"

namespace YAML
{
	class Emitter;
	class Node;
}

namespace CB
{
	class VoxelTextureAsset;

	struct VoxelRendererComponent
	{
		// Cached runtime GPU mesh (generated from vmesh grid)
		Ref<Mesh> MeshAsset;

		// Voxel-specific asset UUIDs
		UUID VoxelMeshUUID = UUID(0); // .vmesh asset UUID
		UUID VoxelTextureUUID = UUID(0); // .vtex asset UUID

		// Palette textures (for GPU rendering)
		Ref<Texture2D> PaletteColorTexture;
		Ref<Texture2D> PaletteMaterialTexture;
		bool HasPalette = false;

		// Voxel Texture (.vtex) - per-voxel material type data + PBR overrides
		Ref<VoxelTextureAsset> VoxelTexture;

		bool Visible = true;

		static constexpr auto YAMLKey = "VoxelRendererComponent";

		VoxelRendererComponent() = default;
		VoxelRendererComponent(const VoxelRendererComponent&) = default;

		void Serialize(YAML::Emitter& out) const;
		void Deserialize(const YAML::Node& node);
		void ResolveAssets();
	};
}