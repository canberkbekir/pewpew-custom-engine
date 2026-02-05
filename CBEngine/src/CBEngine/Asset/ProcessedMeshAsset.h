#pragma once

#include "Asset.h"
#include "CBEngine/Core/Core.h"
#include "CBEngine/Core/String.h"
#include "CBEngine/Core/UUID.h"
#include "CBEngine/Utils/VoxelizerAPI.h"

#include <Voxelizer.h>
#include <vector>
#include <filesystem>

namespace CB
{
    struct MaterialSlot
    {
        String Name;
        UUID MaterialUUID;
    };

    struct TextureSlot
    {
        String Name;
        UUID TextureUUID;
    };

    struct MeshImportSettings
    {
        bool GenerateNormals = true;
        bool CalcTangents = true;
        bool JoinIdenticalVertices = true;
        bool PreTransformVertices = true;
        float Scale = 1.0f;
    };

    class ProcessedMeshAsset : public Asset
    {
    public:
        ProcessedMeshAsset();
        ~ProcessedMeshAsset() override = default;

        // Source mesh info
        String SourceFilePath;
        UUID SourceMeshUUID;
        MeshImportSettings ImportSettings;

        // Slots
        std::vector<MaterialSlot> MaterialSlots;
        std::vector<TextureSlot> TextureSlots;

        // Serialization
        bool Save(const std::filesystem::path& filePath);
        static Ref<ProcessedMeshAsset> Load(const std::filesystem::path& filePath);

        bool Reload() override;

        static AssetType GetStaticType() { return AssetType::ProcessedMesh; }
    };

    class VoxelMeshAsset : public Asset
    {
    public:
        VoxelMeshAsset();
        ~VoxelMeshAsset() override = default;

        // Source mesh info
        String SourceFilePath;
        UUID SourceMeshUUID;
        MeshImportSettings ImportSettings;

        // Voxel-specific
        VoxelizeSettings VoxelSettings;
        UUID ColorTextureUUID;
        voxelizer::VoxelGrid GridData;
        uint64_t VoxelCount = 0;

        // Slots
        std::vector<MaterialSlot> MaterialSlots;
        std::vector<TextureSlot> TextureSlots;

        // Serialization
        bool Save(const std::filesystem::path& filePath);
        static Ref<VoxelMeshAsset> Load(const std::filesystem::path& filePath);

        bool Reload() override;

        static AssetType GetStaticType() { return AssetType::VoxelMesh; }

    private:
        // Base64 encoding/decoding for voxel grid data
        static String EncodeVoxelData(const voxelizer::VoxelData& data);
        static voxelizer::VoxelData DecodeVoxelData(const String& base64);
    };
}
