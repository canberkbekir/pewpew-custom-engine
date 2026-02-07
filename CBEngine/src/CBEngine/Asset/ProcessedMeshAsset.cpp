#include "cbpch.h"
#include "ProcessedMeshAsset.h"
#include "CBEngine/Core/Log.h"
#include "CBEngine/Utils/BinaryIO.h"

namespace CB
{
    // ============================================================================
    // MaterialSlot / TextureSlot serialization
    // ============================================================================

    void MaterialSlot::Serialize(BinaryWriter& writer) const
    {
        writer << Name;
        writer << (uint64_t)MaterialUUID;
    }

    void MaterialSlot::Deserialize(BinaryReader& reader)
    {
        reader >> Name;
        uint64_t uuid = 0;
        reader >> uuid;
        MaterialUUID = UUID(uuid);
    }

    void TextureSlot::Serialize(BinaryWriter& writer) const
    {
        writer << Name;
        writer << (uint64_t)TextureUUID;
    }

    void TextureSlot::Deserialize(BinaryReader& reader)
    {
        reader >> Name;
        uint64_t uuid = 0;
        reader >> uuid;
        TextureUUID = UUID(uuid);
    }

    // ============================================================================
    // ProcessedMeshAsset
    // ============================================================================

    ProcessedMeshAsset::ProcessedMeshAsset()
    {
        m_Type = AssetType::ProcessedMesh;
    }

    bool ProcessedMeshAsset::Save(const std::filesystem::path& filePath)
    {
        FileStream stream(filePath.string().c_str(), "wb");
        if (!stream.IsOpen())
        {
            CB_CORE_ERROR("ProcessedMeshAsset: Failed to save to {0}", filePath.string());
            return false;
        }

        BinaryWriter writer(stream);

        // Header
        writer << s_MeshMagic;
        writer << s_MeshVersion;
        uint32_t flags = 0;
        writer << flags;

        // Source
        writer << SourceFilePath;
        writer << (uint64_t)SourceMeshUUID;

        // Import settings
        writer << (uint8_t)ImportSettings.GenerateNormals;
        writer << (uint8_t)ImportSettings.CalcTangents;
        writer << (uint8_t)ImportSettings.JoinIdenticalVertices;
        writer << (uint8_t)ImportSettings.PreTransformVertices;
        writer << ImportSettings.Scale;

        // Slots
        writer << MaterialSlots;
        writer << TextureSlots;

        // Geometry
        uint32_t vertexCount = static_cast<uint32_t>(Vertices.size());
        uint32_t indexCount = static_cast<uint32_t>(Indices.size());
        writer << vertexCount;
        writer << indexCount;

        if (vertexCount > 0)
            writer.WriteBytes(Vertices.data(), vertexCount * sizeof(Vertex));
        if (indexCount > 0)
            writer.WriteBytes(Indices.data(), indexCount * sizeof(uint32_t));

        return true;
    }

    Ref<ProcessedMeshAsset> ProcessedMeshAsset::Load(const std::filesystem::path& filePath)
    {
        FileStream stream(filePath.string().c_str(), "rb");
        if (!stream.IsOpen())
        {
            CB_CORE_ERROR("ProcessedMeshAsset: Failed to open {0}", filePath.string());
            return nullptr;
        }

        BinaryReader reader(stream);

        // Header
        uint32_t magic = 0, version = 0, flags = 0;
        reader >> magic;
        if (magic != s_MeshMagic)
        {
            CB_CORE_ERROR("ProcessedMeshAsset: Invalid magic in {0} (expected 0x{1:08X}, got 0x{2:08X})",
                filePath.string(), s_MeshMagic, magic);
            return nullptr;
        }
        reader >> version;
        if (version != s_MeshVersion)
        {
            CB_CORE_ERROR("ProcessedMeshAsset: Unsupported version {0} in {1}", version, filePath.string());
            return nullptr;
        }
        reader >> flags;

        auto asset = CreateRef<ProcessedMeshAsset>();
        asset->m_Path = filePath;

        // Source
        reader >> asset->SourceFilePath;
        uint64_t sourceUUID = 0;
        reader >> sourceUUID;
        asset->SourceMeshUUID = UUID(sourceUUID);

        // Import settings
        uint8_t genNormals = 0, calcTangents = 0, joinVerts = 0, preTransform = 0;
        reader >> genNormals;
        reader >> calcTangents;
        reader >> joinVerts;
        reader >> preTransform;
        reader >> asset->ImportSettings.Scale;
        asset->ImportSettings.GenerateNormals = genNormals != 0;
        asset->ImportSettings.CalcTangents = calcTangents != 0;
        asset->ImportSettings.JoinIdenticalVertices = joinVerts != 0;
        asset->ImportSettings.PreTransformVertices = preTransform != 0;

        // Slots
        reader >> asset->MaterialSlots;
        reader >> asset->TextureSlots;

        // Geometry
        uint32_t vertexCount = 0, indexCount = 0;
        reader >> vertexCount;
        reader >> indexCount;

        if (vertexCount > 0)
        {
            asset->Vertices.resize(vertexCount);
            reader.ReadBytes(asset->Vertices.data(), vertexCount * sizeof(Vertex));
        }
        if (indexCount > 0)
        {
            asset->Indices.resize(indexCount);
            reader.ReadBytes(asset->Indices.data(), indexCount * sizeof(uint32_t));
        }

        return asset;
    }

    Ref<Mesh> ProcessedMeshAsset::CreateMesh() const
    {
        if (!HasGeometryData())
        {
            CB_CORE_WARN("ProcessedMeshAsset::CreateMesh: No geometry data embedded");
            return nullptr;
        }

        return CreateRef<Mesh>(Vertices, Indices);
    }

    bool ProcessedMeshAsset::Reload()
    {
        if (m_Path.empty())
            return false;

        auto reloaded = Load(m_Path);
        if (!reloaded)
            return false;

        SourceFilePath = reloaded->SourceFilePath;
        SourceMeshUUID = reloaded->SourceMeshUUID;
        ImportSettings = reloaded->ImportSettings;
        MaterialSlots = reloaded->MaterialSlots;
        TextureSlots = reloaded->TextureSlots;
        Vertices = std::move(reloaded->Vertices);
        Indices = std::move(reloaded->Indices);
        return true;
    }

    // ============================================================================
    // VoxelMeshAsset
    // ============================================================================

    VoxelMeshAsset::VoxelMeshAsset()
    {
        m_Type = AssetType::VoxelMesh;
    }

    bool VoxelMeshAsset::Save(const std::filesystem::path& filePath)
    {
        FileStream stream(filePath.string().c_str(), "wb");
        if (!stream.IsOpen())
        {
            CB_CORE_ERROR("VoxelMeshAsset: Failed to save to {0}", filePath.string());
            return false;
        }

        BinaryWriter writer(stream);

        // Header
        writer << s_VmeshMagic;
        writer << s_VmeshVersion;
        uint32_t flags = 0;
        writer << flags;

        // Source
        writer << SourceFilePath;
        writer << (uint64_t)SourceMeshUUID;

        // Import settings
        writer << (uint8_t)ImportSettings.GenerateNormals;
        writer << (uint8_t)ImportSettings.CalcTangents;
        writer << (uint8_t)ImportSettings.JoinIdenticalVertices;
        writer << (uint8_t)ImportSettings.PreTransformVertices;
        writer << ImportSettings.Scale;

        // Voxel settings
        writer << VoxelSettings.GridSize;
        writer << VoxelSettings.VoxelSize;
        writer << (uint8_t)VoxelSettings.Solid;
        writer << VoxelSettings.Padding;
        writer << (uint64_t)ColorTextureUUID;

        // Slots
        writer << MaterialSlots;
        writer << TextureSlots;

        // Grid data
        writer << GridData.size.x;
        writer << GridData.size.y;
        writer << GridData.size.z;
        writer << GridData.origin.x;
        writer << GridData.origin.y;
        writer << GridData.origin.z;
        writer << GridData.voxelSize.x;
        writer << GridData.voxelSize.y;
        writer << GridData.voxelSize.z;
        writer << GridData.totalVoxels;
        writer << VoxelCount;
        writer << GridData.data; // vector<uint64_t> - uses BinaryIO vector serialization

        return true;
    }

    Ref<VoxelMeshAsset> VoxelMeshAsset::Load(const std::filesystem::path& filePath)
    {
        FileStream stream(filePath.string().c_str(), "rb");
        if (!stream.IsOpen())
        {
            CB_CORE_ERROR("VoxelMeshAsset: Failed to open {0}", filePath.string());
            return nullptr;
        }

        BinaryReader reader(stream);

        // Header
        uint32_t magic = 0, version = 0, flags = 0;
        reader >> magic;
        if (magic != s_VmeshMagic)
        {
            CB_CORE_ERROR("VoxelMeshAsset: Invalid magic in {0} (expected 0x{1:08X}, got 0x{2:08X})",
                filePath.string(), s_VmeshMagic, magic);
            return nullptr;
        }
        reader >> version;
        if (version != s_VmeshVersion)
        {
            CB_CORE_ERROR("VoxelMeshAsset: Unsupported version {0} in {1}", version, filePath.string());
            return nullptr;
        }
        reader >> flags;

        auto asset = CreateRef<VoxelMeshAsset>();
        asset->m_Path = filePath;

        // Source
        reader >> asset->SourceFilePath;
        uint64_t sourceUUID = 0;
        reader >> sourceUUID;
        asset->SourceMeshUUID = UUID(sourceUUID);

        // Import settings
        uint8_t genNormals = 0, calcTangents = 0, joinVerts = 0, preTransform = 0;
        reader >> genNormals;
        reader >> calcTangents;
        reader >> joinVerts;
        reader >> preTransform;
        reader >> asset->ImportSettings.Scale;
        asset->ImportSettings.GenerateNormals = genNormals != 0;
        asset->ImportSettings.CalcTangents = calcTangents != 0;
        asset->ImportSettings.JoinIdenticalVertices = joinVerts != 0;
        asset->ImportSettings.PreTransformVertices = preTransform != 0;

        // Voxel settings
        reader >> asset->VoxelSettings.GridSize;
        reader >> asset->VoxelSettings.VoxelSize;
        uint8_t solid = 0;
        reader >> solid;
        asset->VoxelSettings.Solid = solid != 0;
        reader >> asset->VoxelSettings.Padding;
        uint64_t colorTexUUID = 0;
        reader >> colorTexUUID;
        asset->ColorTextureUUID = UUID(colorTexUUID);

        // Slots
        reader >> asset->MaterialSlots;
        reader >> asset->TextureSlots;

        // Grid data
        reader >> asset->GridData.size.x;
        reader >> asset->GridData.size.y;
        reader >> asset->GridData.size.z;
        reader >> asset->GridData.origin.x;
        reader >> asset->GridData.origin.y;
        reader >> asset->GridData.origin.z;
        reader >> asset->GridData.voxelSize.x;
        reader >> asset->GridData.voxelSize.y;
        reader >> asset->GridData.voxelSize.z;
        reader >> asset->GridData.totalVoxels;
        reader >> asset->VoxelCount;
        reader >> asset->GridData.data; // vector<uint64_t>

        return asset;
    }

    bool VoxelMeshAsset::Reload()
    {
        if (m_Path.empty())
            return false;

        auto reloaded = Load(m_Path);
        if (!reloaded)
            return false;

        SourceFilePath = reloaded->SourceFilePath;
        SourceMeshUUID = reloaded->SourceMeshUUID;
        ImportSettings = reloaded->ImportSettings;
        VoxelSettings = reloaded->VoxelSettings;
        ColorTextureUUID = reloaded->ColorTextureUUID;
        GridData = reloaded->GridData;
        VoxelCount = reloaded->VoxelCount;
        MaterialSlots = reloaded->MaterialSlots;
        TextureSlots = reloaded->TextureSlots;
        return true;
    }
}
