#include "cbpch.h"
#include "VoxelRendererComponent.h"
#include "CBEngine/Asset/AssetManager.h"
#include "CBEngine/Asset/ProcessedMeshAsset.h"
#include "CBEngine/Asset/VoxelTextureAsset.h"
#include "CBEngine/Utils/VoxelizerAPI.h"

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

        if (VoxelMeshUUID.IsValid())
        {
            auto vmAsset = AssetManager::GetAsset<VoxelMeshAsset>(VoxelMeshUUID);
            if (vmAsset && vmAsset->VoxelCount > 0)
            {
                if (vmAsset->HasPalette && !vmAsset->PaletteIndices.empty())
                {
                    // Start with vmesh palette as base
                    VoxelPalette palette = vmAsset->Palette;

                    // Apply VTex PBR overrides if available
                    if (VoxelTexture)
                    {
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
                else
                {
                    // No palette data - create white mesh
                    MeshAsset = VoxelizerAPI::CreateMeshFromGrid(vmAsset->GridData);
                    HasPalette = false;
                }
            }
        }
    }
}
