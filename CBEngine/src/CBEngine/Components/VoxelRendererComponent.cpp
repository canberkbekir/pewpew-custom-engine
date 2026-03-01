#include "cbpch.h"
#include "VoxelRendererComponent.h"
#include "CBEngine/Asset/AssetManager.h"
#include "CBEngine/Asset/ProcessedMeshAsset.h"
#include "CBEngine/Asset/VoxelTextureAsset.h"
#include "CBEngine/Voxel/VoxelizerAPI.h"

#include <yaml-cpp/yaml.h>

namespace CB
{
	void VoxelRendererComponent::Serialize(YAML::Emitter& out) const
	{
		out << YAML::Key << "VoxelMeshUUID" << YAML::Value << VoxelMeshUUID;
		out << YAML::Key << "VoxelTextureUUID" << YAML::Value << VoxelTextureUUID;
		out << YAML::Key << "Visible" << YAML::Value << Visible;
	}

	void VoxelRendererComponent::Deserialize(const YAML::Node& node)
	{
		if (node["VoxelMeshUUID"])
			VoxelMeshUUID = UUID(node["VoxelMeshUUID"].as<uint64_t>());
		if (node["VoxelTextureUUID"])
			VoxelTextureUUID = UUID(node["VoxelTextureUUID"].as<uint64_t>());
		if (node["Visible"])
			Visible = node["Visible"].as<bool>();
	}

	void VoxelRendererComponent::ResolveAssets()
	{
		// Load vtex first (needed for PBR overrides)
		if (VoxelTextureUUID.IsValid())
			VoxelTexture = AssetManager::GetAsset<VoxelTextureAsset>(VoxelTextureUUID);

		if (VoxelMeshUUID.IsValid()) {
			auto vmAsset = AssetManager::GetAsset<VoxelMeshAsset>(VoxelMeshUUID);
			if (vmAsset && vmAsset->VoxelCount > 0) {
				// If vmesh has no palette, generate a default one so vtex overrides work
				if (!vmAsset->HasPalette || vmAsset->PaletteIndices.empty()) {
					VoxelPaletteEntry defaultEntry;
					defaultEntry.Color = Vector3(0.7f, 0.7f, 0.7f);
					defaultEntry.Metallic = 0.0f;
					defaultEntry.Roughness = 0.5f;
					defaultEntry.Emission = 0.0f;
					vmAsset->Palette.SetEntry(0, defaultEntry);
					vmAsset->PaletteIndices.resize(vmAsset->VoxelCount, 0);
					vmAsset->HasPalette = true;
				}

				// Start with vmesh palette as base
				VoxelPalette palette = vmAsset->Palette;

				// Apply VTex PBR overrides if available
				if (VoxelTexture) {
					bool hasAnyOverride = VoxelTexture->HasMetallicOverrides
					|| VoxelTexture->HasRoughnessOverrides
					|| VoxelTexture->HasEmissionOverrides
					|| VoxelTexture->HasAlbedoOverrides;

					if (hasAnyOverride)
						palette = VoxelTexture->ApplyOverrides(palette);
				}

				MeshAsset = VoxelizerAPI::CreatePaletteMeshFromGrid(vmAsset->GridData, vmAsset->PaletteIndices);

				std::vector<uint8_t> colorData, materialData;
				palette.GenerateColorTextureData(colorData);
				palette.GenerateMaterialTextureData(materialData);

				PaletteColorTexture = Texture2D::CreateFromData(256, 1, colorData.data(), true);
				PaletteMaterialTexture = Texture2D::CreateFromData(256, 1, materialData.data(), true);
				HasPalette = true;
			}
		}

		// Resolve dominant substance from vtex palette mapping
		if (VoxelTexture && !VoxelTexture->PaletteMapping.empty()) {
			// Use the first palette entry's substance (covers "Set All" and single-substance cases)
			DominantSubstanceID = VoxelTexture->PaletteMapping.begin()->second;
		}
	}
}