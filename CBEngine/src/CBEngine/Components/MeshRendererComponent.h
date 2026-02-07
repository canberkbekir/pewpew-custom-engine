#pragma once

#include "CBEngine/Core/Core.h"
#include "CBEngine/Core/UUID.h"
#include "CBEngine/Renderer/Resources/Mesh.h"
#include "CBEngine/Renderer/Resources/Material.h"
#include "CBEngine/Renderer/Resources/Shader.h"

namespace YAML { class Emitter; class Node; }

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

		static constexpr const char* YAMLKey = "MeshRendererComponent";

		MeshRendererComponent() = default;
		MeshRendererComponent(const MeshRendererComponent&) = default;

		void Serialize(YAML::Emitter& out) const;
		void Deserialize(const YAML::Node& node);
		void ResolveAssets();
	};

}
