#pragma once

#include "CBEngine/Core/Core.h"
#include "CBEngine/Core/UUID.h"
#include "CBEngine/Renderer/Resources/Mesh.h"
#include "CBEngine/Renderer/Resources/Material.h"
#include "CBEngine/Renderer/Resources/Shader.h"
#include "CBEngine/Utils/VoxelizerAPI.h"

namespace CB {

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
		VoxelizeSettings VoxelSettings;

		bool Visible = true;

		VoxelRendererComponent() = default;
		VoxelRendererComponent(const VoxelRendererComponent&) = default;
	};

}
