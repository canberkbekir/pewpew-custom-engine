#pragma once

#include "Asset.h"
#include "CBEngine/Core/Core.h"
#include "CBEngine/Core/String.h"
#include "CBEngine/Core/UUID.h"
#include "CBEngine/Renderer/Resources/Mesh.h"
#include "CBEngine/Utils/VoxelizerAPI.h"
#include "CBEngine/Utils/VoxelPalette.h"

#include <Voxelizer.h>
#include <vector>
#include <filesystem>

namespace CB
{
    class BinaryWriter;
    class BinaryReader;

    struct MaterialSlot
    {
        String Name;
        UUID MaterialUUID;

        void Serialize(BinaryWriter& writer) const;
        void Deserialize(BinaryReader& reader);
    };

    struct TextureSlot
    {
        String Name;
        UUID TextureUUID;

        void Serialize(BinaryWriter& writer) const;
        void Deserialize(BinaryReader& reader);
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
        static constexpr uint32_t s_MeshMagic = 0x43424D48; // "CBMH"
        static constexpr uint32_t s_MeshVersion = 2;

        ProcessedMeshAsset();
        ~ProcessedMeshAsset() override = default;

        // Source mesh info
        String SourceFilePath;
        UUID SourceMeshUUID;
        MeshImportSettings ImportSettings;

        // Slots
        std::vector<MaterialSlot> MaterialSlots;
        std::vector<TextureSlot> TextureSlots;

        // Embedded geometry (no FBX needed at runtime)
        std::vector<Vertex> Vertices;
        std::vector<uint32_t> Indices;

        // Create a renderable Mesh from embedded geometry
        Ref<Mesh> CreateMesh() const;
        bool HasGeometryData() const { return !Vertices.empty() && !Indices.empty(); }

        // Serialization
        bool Save(const std::filesystem::path& filePath);
        static Ref<ProcessedMeshAsset> Load(const std::filesystem::path& filePath);

        bool Reload() override;

        static AssetType GetStaticType() { return AssetType::ProcessedMesh; }
    };

    class VoxelMeshAsset : public Asset
    {
    public:
        static constexpr uint32_t s_VmeshMagic = 0x43425648; // "CBVM"
        static constexpr uint32_t s_VmeshVersion = 3;

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

        // Palette data (v2+)
        VoxelPalette Palette;
        std::vector<uint8_t> PaletteIndices;
        bool HasPalette = false;

        // Per-voxel UVs for deferred texture sampling (v3+)
        std::vector<Vector2> VoxelUVs;
        bool HasUVs = false;

        // Slots
        std::vector<MaterialSlot> MaterialSlots;
        std::vector<TextureSlot> TextureSlots;

        // Serialization
        bool Save(const std::filesystem::path& filePath);
        static Ref<VoxelMeshAsset> Load(const std::filesystem::path& filePath);

        bool Reload() override;

        static AssetType GetStaticType() { return AssetType::VoxelMesh; }
    };
}
