#pragma once

#include "CBEngine/Core/Core.h"
#include "CBEngine/Core/UUID.h"
#include "CBEngine/Renderer/Resources/Mesh.h"
#include "CBEngine/Renderer/Resources/Material.h"
#include "CBEngine/Renderer/Resources/Shader.h"

namespace CB {

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