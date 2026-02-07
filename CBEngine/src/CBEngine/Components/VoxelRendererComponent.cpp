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
		out << YAML::Key << "ColorTextureUUID" << YAML::Value << (uint64_t)ColorTextureUUID;
		out << YAML::Key << "MaterialUUID" << YAML::Value << (uint64_t)MaterialUUID;
		out << YAML::Key << "ShaderUUID" << YAML::Value << (uint64_t)ShaderUUID;
		out << YAML::Key << "Visible" << YAML::Value << Visible;
	}

	void VoxelRendererComponent::Deserialize(const YAML::Node& node)
	{
		VoxelMeshUUID = UUID(node["VoxelMeshUUID"].as<uint64_t>());
		if (node["ColorTextureUUID"])
			ColorTextureUUID = UUID(node["ColorTextureUUID"].as<uint64_t>());
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
				VoxelSettings = vmAsset->VoxelSettings;

				bool paletteBuilt = false;

				// Try to build palette from UVs + assigned color texture
				if (vmAsset->HasUVs && !vmAsset->VoxelUVs.empty() && ColorTextureUUID.IsValid())
				{
					// Get texture file path from asset registry
					const AssetMetadata* texMeta = AssetManager::GetRegistry().GetMetadata(ColorTextureUUID);
					if (texMeta)
					{
						std::filesystem::path texPath = AssetManager::GetAssetDirectory() / texMeta->FilePath;
						auto paletteData = VoxelizerAPI::BuildPaletteFromUVs(vmAsset->VoxelUVs, texPath.string());

						if (!paletteData.PaletteIndices.empty())
						{
							MeshAsset = VoxelizerAPI::CreatePaletteMeshFromGrid(vmAsset->GridData, paletteData.PaletteIndices);

							std::vector<uint8_t> colorData, materialData;
							paletteData.Palette.GenerateColorTextureData(colorData);
							paletteData.Palette.GenerateMaterialTextureData(materialData);

							PaletteColorTexture = Texture2D::CreateFromData(256, 1, colorData.data(), true);
							PaletteMaterialTexture = Texture2D::CreateFromData(256, 1, materialData.data(), true);
							HasPalette = true;
							paletteBuilt = true;
						}
					}
				}

				// Fallback: use embedded palette from vmesh (if baked during import)
				if (!paletteBuilt && vmAsset->HasPalette && !vmAsset->PaletteIndices.empty())
				{
					MeshAsset = VoxelizerAPI::CreatePaletteMeshFromGrid(vmAsset->GridData, vmAsset->PaletteIndices);

					std::vector<uint8_t> colorData, materialData;
					vmAsset->Palette.GenerateColorTextureData(colorData);
					vmAsset->Palette.GenerateMaterialTextureData(materialData);

					PaletteColorTexture = Texture2D::CreateFromData(256, 1, colorData.data(), true);
					PaletteMaterialTexture = Texture2D::CreateFromData(256, 1, materialData.data(), true);
					HasPalette = true;
					paletteBuilt = true;
				}

				// Final fallback: create white mesh (old vmesh with no palette/UVs)
				if (!paletteBuilt)
				{
					MeshAsset = VoxelizerAPI::CreateMeshFromGrid(vmAsset->GridData);
					HasPalette = false;
				}
			}
		}
		if (MaterialUUID.IsValid())
			MaterialAsset = AssetManager::GetAsset<Material>(MaterialUUID);
		if (ShaderUUID.IsValid())
			ShaderAsset = AssetManager::GetAsset<Shader>(ShaderUUID);
	}
}
