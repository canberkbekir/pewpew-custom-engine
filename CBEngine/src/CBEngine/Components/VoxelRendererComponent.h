#pragma once

#include "CBEngine/Core/Core.h"
#include "CBEngine/Core/UUID.h"
#include "CBEngine/Renderer/Resources/Mesh.h"
#include "CBEngine/Renderer/Resources/Material.h"
#include "CBEngine/Renderer/Resources/Shader.h"
#include "CBEngine/Renderer/Resources/Texture.h"
#include "CBEngine/Utils/VoxelizerAPI.h"

namespace YAML { class Emitter; class Node; }

namespace CB {

	class VoxelTextureAsset;

	struct VoxelRendererComponent
	{
		// Cached asset references (loaded from UUIDs)
		Ref<Mesh> MeshAsset;
		Ref<Material> MaterialAsset;
		Ref<Shader> ShaderAsset;

		// Asset UUIDs for serialization and picker
		UUID MeshUUID = UUID(0);       // Not used directly; mesh is generated from voxel grid
		UUID MaterialUUID = UUID(0);
		UUID ShaderUUID = UUID(0);

		// Voxel-specific
		UUID VoxelMeshUUID = UUID(0);  // .vmesh asset UUID
		UUID ColorTextureUUID = UUID(0); // Texture to sample for voxel colors
		VoxelizeSettings VoxelSettings;

		// Palette textures (for GPU rendering)
		Ref<Texture2D> PaletteColorTexture;
		Ref<Texture2D> PaletteMaterialTexture;
		bool HasPalette = false;

		// Voxel Texture (.vtex) - per-voxel material type data
		UUID VoxelTextureUUID = UUID(0);
		Ref<VoxelTextureAsset> VoxelTexture;

		bool Visible = true;

		static constexpr const char* YAMLKey = "VoxelRendererComponent";

		VoxelRendererComponent() = default;
		VoxelRendererComponent(const VoxelRendererComponent&) = default;

		void Serialize(YAML::Emitter& out) const;
		void Deserialize(const YAML::Node& node);
		void ResolveAssets();
	};

}
