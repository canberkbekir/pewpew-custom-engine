#pragma once

#include "PewPew/Core/Core.h"
#include "PewPew/Core/UUID.h"
#include "PewPew/Renderer/Resources/Mesh.h"
#include "PewPew/Renderer/Resources/Material.h"
#include "PewPew/Renderer/Resources/Shader.h"

namespace PewPew {

	struct MeshRendererComponent
	{
		// Cached asset references (loaded from UUIDs)
		Ref<Mesh> MeshAsset;
		Ref<Material> MaterialAsset;
		Ref<Shader> ShaderAsset;

		// Asset UUIDs for serialization and picker
		UUID MeshUUID = UUID(0);
		UUID MaterialUUID = UUID(0);
		UUID ShaderUUID = UUID(0);

		bool Visible = true;

		MeshRendererComponent() = default;
		MeshRendererComponent(const MeshRendererComponent&) = default;
	};

}