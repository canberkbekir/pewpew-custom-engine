#include "cbpch.h"
#include "VoxelRendererComponent.h"
#include "CBEngine/Asset/AssetManager.h"
#include "CBEngine/Asset/ProcessedMeshAsset.h"

#include <yaml-cpp/yaml.h>

namespace CB
{
	void VoxelRendererComponent::Serialize(YAML::Emitter& out) const
	{
		out << YAML::Key << "VoxelMeshUUID" << YAML::Value << (uint64_t)VoxelMeshUUID;
		out << YAML::Key << "MaterialUUID" << YAML::Value << (uint64_t)MaterialUUID;
		out << YAML::Key << "ShaderUUID" << YAML::Value << (uint64_t)ShaderUUID;
		out << YAML::Key << "Visible" << YAML::Value << Visible;
	}

	void VoxelRendererComponent::Deserialize(const YAML::Node& node)
	{
		VoxelMeshUUID = UUID(node["VoxelMeshUUID"].as<uint64_t>());
		MaterialUUID = UUID(node["MaterialUUID"].as<uint64_t>());
		ShaderUUID = UUID(node["ShaderUUID"].as<uint64_t>());
		Visible = node["Visible"].as<bool>();
	}

	void VoxelRendererComponent::ResolveAssets()
	{
		if (VoxelMeshUUID.IsValid())
		{
			auto vmAsset = AssetManager::GetAsset<VoxelMeshAsset>(VoxelMeshUUID);
			if (vmAsset && vmAsset->VoxelCount > 0)
			{
				MeshAsset = VoxelizerAPI::CreateMeshFromGrid(vmAsset->GridData);
				VoxelSettings = vmAsset->VoxelSettings;
			}
		}
		if (MaterialUUID.IsValid())
			MaterialAsset = AssetManager::GetAsset<Material>(MaterialUUID);
		if (ShaderUUID.IsValid())
			ShaderAsset = AssetManager::GetAsset<Shader>(ShaderUUID);
	}
}
