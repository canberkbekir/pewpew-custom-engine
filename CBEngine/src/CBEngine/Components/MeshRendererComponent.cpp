#include "cbpch.h"
#include "MeshRendererComponent.h"
#include "CBEngine/Asset/AssetManager.h"
#include "CBEngine/Asset/ProcessedMeshAsset.h"

#include <yaml-cpp/yaml.h>

namespace CB
{
    void MeshRendererComponent::Serialize(YAML::Emitter& out) const
    {
        out << YAML::Key << "MeshUUID" << YAML::Value << MeshUUID;
        out << YAML::Key << "MaterialUUID" << YAML::Value << MaterialUUID;
        out << YAML::Key << "ShaderUUID" << YAML::Value << ShaderUUID;
        out << YAML::Key << "Visible" << YAML::Value << Visible;
    }

    void MeshRendererComponent::Deserialize(const YAML::Node& node)
    {
        MeshUUID = UUID(node["MeshUUID"].as<uint64_t>());
        MaterialUUID = UUID(node["MaterialUUID"].as<uint64_t>());
        ShaderUUID = UUID(node["ShaderUUID"].as<uint64_t>());
        Visible = node["Visible"].as<bool>();
    }

    void MeshRendererComponent::ResolveAssets()
    {
        if (MeshUUID.IsValid())
        {
            const AssetMetadata* meta = AssetManager::GetRegistry().GetMetadata(MeshUUID);
            if (meta && meta->Type == AssetType::ProcessedMesh)
            {
                auto pmAsset = AssetManager::GetAsset<ProcessedMeshAsset>(MeshUUID);
                if (pmAsset && pmAsset->HasGeometryData())
                    MeshAsset = pmAsset->CreateMesh();
                else if (pmAsset && !pmAsset->SourceFilePath.empty())
                    MeshAsset = Mesh::Load((AssetManager::GetAssetDirectory() / pmAsset->SourceFilePath).string());
            }
            else
            {
                MeshAsset = AssetManager::GetAsset<Mesh>(MeshUUID);
            }
        }
        if (MaterialUUID.IsValid())
            MaterialAsset = AssetManager::GetAsset<Material>(MaterialUUID);
        if (ShaderUUID.IsValid())
            ShaderAsset = AssetManager::GetAsset<Shader>(ShaderUUID);
    }
}
