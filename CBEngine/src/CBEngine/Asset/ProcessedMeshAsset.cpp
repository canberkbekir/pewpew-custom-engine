#include "cbpch.h"
#include "ProcessedMeshAsset.h"
#include "CBEngine/Core/Log.h"

#include <yaml-cpp/yaml.h>
#include <fstream>

namespace CB
{
    // ============================================================================
    // Base64 Encoding/Decoding for voxel grid data
    // ============================================================================

    static const char Base64Chars[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    static inline bool IsBase64(unsigned char c)
    {
        return (isalnum(c) || (c == '+') || (c == '/'));
    }

    static String Base64Encode(const unsigned char* bytes, size_t length)
    {
        String result;
        result.reserve(((length + 2) / 3) * 4);

        int i = 0;
        unsigned char array3[3];
        unsigned char array4[4];

        while (length--)
        {
            array3[i++] = *(bytes++);
            if (i == 3)
            {
                array4[0] = (array3[0] & 0xfc) >> 2;
                array4[1] = ((array3[0] & 0x03) << 4) + ((array3[1] & 0xf0) >> 4);
                array4[2] = ((array3[1] & 0x0f) << 2) + ((array3[2] & 0xc0) >> 6);
                array4[3] = array3[2] & 0x3f;

                for (i = 0; i < 4; i++)
                    result += Base64Chars[array4[i]];
                i = 0;
            }
        }

        if (i)
        {
            for (int j = i; j < 3; j++)
                array3[j] = '\0';

            array4[0] = (array3[0] & 0xfc) >> 2;
            array4[1] = ((array3[0] & 0x03) << 4) + ((array3[1] & 0xf0) >> 4);
            array4[2] = ((array3[1] & 0x0f) << 2) + ((array3[2] & 0xc0) >> 6);
            array4[3] = array3[2] & 0x3f;

            for (int j = 0; j < (i + 1); j++)
                result += Base64Chars[array4[j]];

            while (i++ < 3)
                result += '=';
        }

        return result;
    }

    static std::vector<unsigned char> Base64Decode(const String& encoded)
    {
        size_t length = encoded.size();
        int i = 0;
        int in_ = 0;
        unsigned char array4[4], array3[3];
        std::vector<unsigned char> result;

        while (length-- && (encoded[in_] != '=') && IsBase64(encoded[in_]))
        {
            array4[i++] = encoded[in_]; in_++;
            if (i == 4)
            {
                for (i = 0; i < 4; i++)
                    array4[i] = static_cast<unsigned char>(
                        String(Base64Chars).find(array4[i]));

                array3[0] = (array4[0] << 2) + ((array4[1] & 0x30) >> 4);
                array3[1] = ((array4[1] & 0xf) << 4) + ((array4[2] & 0x3c) >> 2);
                array3[2] = ((array4[2] & 0x3) << 6) + array4[3];

                for (i = 0; i < 3; i++)
                    result.push_back(array3[i]);
                i = 0;
            }
        }

        if (i)
        {
            for (int j = i; j < 4; j++)
                array4[j] = 0;

            for (int j = 0; j < 4; j++)
                array4[j] = static_cast<unsigned char>(
                    String(Base64Chars).find(array4[j]));

            array3[0] = (array4[0] << 2) + ((array4[1] & 0x30) >> 4);
            array3[1] = ((array4[1] & 0xf) << 4) + ((array4[2] & 0x3c) >> 2);
            array3[2] = ((array4[2] & 0x3) << 6) + array4[3];

            for (int j = 0; j < (i - 1); j++)
                result.push_back(array3[j]);
        }

        return result;
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
        YAML::Emitter out;
        out << YAML::BeginMap;

        out << YAML::Key << "version" << YAML::Value << 1;

        // Source
        out << YAML::Key << "source" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "path" << YAML::Value << SourceFilePath;
        out << YAML::Key << "uuid" << YAML::Value << (uint64_t)SourceMeshUUID;
        out << YAML::EndMap;

        // Import settings
        out << YAML::Key << "import_settings" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "scale" << YAML::Value << ImportSettings.Scale;
        out << YAML::Key << "generate_normals" << YAML::Value << ImportSettings.GenerateNormals;
        out << YAML::Key << "calc_tangents" << YAML::Value << ImportSettings.CalcTangents;
        out << YAML::Key << "join_identical_vertices" << YAML::Value << ImportSettings.JoinIdenticalVertices;
        out << YAML::Key << "pre_transform_vertices" << YAML::Value << ImportSettings.PreTransformVertices;
        out << YAML::EndMap;

        // Material slots
        out << YAML::Key << "material_slots" << YAML::Value << YAML::BeginSeq;
        for (const auto& slot : MaterialSlots)
        {
            out << YAML::BeginMap;
            out << YAML::Key << "name" << YAML::Value << slot.Name;
            out << YAML::Key << "material" << YAML::Value << (uint64_t)slot.MaterialUUID;
            out << YAML::EndMap;
        }
        out << YAML::EndSeq;

        // Texture slots
        out << YAML::Key << "texture_slots" << YAML::Value << YAML::BeginSeq;
        for (const auto& slot : TextureSlots)
        {
            out << YAML::BeginMap;
            out << YAML::Key << "name" << YAML::Value << slot.Name;
            out << YAML::Key << "texture" << YAML::Value << (uint64_t)slot.TextureUUID;
            out << YAML::EndMap;
        }
        out << YAML::EndSeq;

        out << YAML::EndMap;

        std::ofstream fout(filePath);
        if (!fout.is_open())
        {
            CB_CORE_ERROR("ProcessedMeshAsset: Failed to save to {0}", filePath.string());
            return false;
        }

        fout << out.c_str();
        return true;
    }

    Ref<ProcessedMeshAsset> ProcessedMeshAsset::Load(const std::filesystem::path& filePath)
    {
        std::ifstream fin(filePath);
        if (!fin.is_open())
        {
            CB_CORE_ERROR("ProcessedMeshAsset: Failed to open {0}", filePath.string());
            return nullptr;
        }

        YAML::Node data;
        try
        {
            data = YAML::Load(fin);
        }
        catch (const YAML::ParserException& e)
        {
            CB_CORE_ERROR("ProcessedMeshAsset: YAML parse error in {0}: {1}", filePath.string(), e.what());
            return nullptr;
        }

        auto asset = CreateRef<ProcessedMeshAsset>();
        asset->m_Path = filePath;

        // Source
        if (auto source = data["source"])
        {
            asset->SourceFilePath = source["path"].as<std::string>("");
            asset->SourceMeshUUID = UUID(source["uuid"].as<uint64_t>(0));
        }

        // Import settings
        if (auto settings = data["import_settings"])
        {
            asset->ImportSettings.Scale = settings["scale"].as<float>(1.0f);
            asset->ImportSettings.GenerateNormals = settings["generate_normals"].as<bool>(true);
            asset->ImportSettings.CalcTangents = settings["calc_tangents"].as<bool>(true);
            asset->ImportSettings.JoinIdenticalVertices = settings["join_identical_vertices"].as<bool>(true);
            asset->ImportSettings.PreTransformVertices = settings["pre_transform_vertices"].as<bool>(true);
        }

        // Material slots
        if (auto slots = data["material_slots"])
        {
            for (const auto& slotNode : slots)
            {
                MaterialSlot slot;
                slot.Name = slotNode["name"].as<std::string>("");
                slot.MaterialUUID = UUID(slotNode["material"].as<uint64_t>(0));
                asset->MaterialSlots.push_back(slot);
            }
        }

        // Texture slots
        if (auto slots = data["texture_slots"])
        {
            for (const auto& slotNode : slots)
            {
                TextureSlot slot;
                slot.Name = slotNode["name"].as<std::string>("");
                slot.TextureUUID = UUID(slotNode["texture"].as<uint64_t>(0));
                asset->TextureSlots.push_back(slot);
            }
        }

        return asset;
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
        return true;
    }

    // ============================================================================
    // VoxelMeshAsset
    // ============================================================================

    VoxelMeshAsset::VoxelMeshAsset()
    {
        m_Type = AssetType::VoxelMesh;
    }

    String VoxelMeshAsset::EncodeVoxelData(const voxelizer::VoxelData& data)
    {
        if (data.empty())
            return "";

        const unsigned char* bytes = reinterpret_cast<const unsigned char*>(data.data());
        size_t byteCount = data.size() * sizeof(uint64_t);
        return Base64Encode(bytes, byteCount);
    }

    voxelizer::VoxelData VoxelMeshAsset::DecodeVoxelData(const String& base64)
    {
        if (base64.empty())
            return {};

        auto decoded = Base64Decode(base64);
        size_t count = decoded.size() / sizeof(uint64_t);
        voxelizer::VoxelData result(count);
        memcpy(result.data(), decoded.data(), count * sizeof(uint64_t));
        return result;
    }

    bool VoxelMeshAsset::Save(const std::filesystem::path& filePath)
    {
        YAML::Emitter out;
        out << YAML::BeginMap;

        out << YAML::Key << "version" << YAML::Value << 1;

        // Source
        out << YAML::Key << "source" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "path" << YAML::Value << SourceFilePath;
        out << YAML::Key << "uuid" << YAML::Value << (uint64_t)SourceMeshUUID;
        out << YAML::EndMap;

        // Voxel settings
        out << YAML::Key << "voxel_settings" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "grid_size" << YAML::Value << VoxelSettings.GridSize;
        out << YAML::Key << "solid" << YAML::Value << VoxelSettings.Solid;
        out << YAML::Key << "padding" << YAML::Value << VoxelSettings.Padding;
        out << YAML::Key << "voxel_size" << YAML::Value << VoxelSettings.VoxelSize;
        out << YAML::EndMap;

        out << YAML::Key << "color_texture" << YAML::Value << (uint64_t)ColorTextureUUID;

        // Material slots
        out << YAML::Key << "material_slots" << YAML::Value << YAML::BeginSeq;
        for (const auto& slot : MaterialSlots)
        {
            out << YAML::BeginMap;
            out << YAML::Key << "name" << YAML::Value << slot.Name;
            out << YAML::Key << "material" << YAML::Value << (uint64_t)slot.MaterialUUID;
            out << YAML::EndMap;
        }
        out << YAML::EndSeq;

        // Texture slots
        out << YAML::Key << "texture_slots" << YAML::Value << YAML::BeginSeq;
        for (const auto& slot : TextureSlots)
        {
            out << YAML::BeginMap;
            out << YAML::Key << "name" << YAML::Value << slot.Name;
            out << YAML::Key << "texture" << YAML::Value << (uint64_t)slot.TextureUUID;
            out << YAML::EndMap;
        }
        out << YAML::EndSeq;

        // Voxel grid data
        out << YAML::Key << "voxel_grid" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "dimensions" << YAML::Value << YAML::Flow
            << YAML::BeginSeq << GridData.size.x << GridData.size.y << GridData.size.z << YAML::EndSeq;
        out << YAML::Key << "origin" << YAML::Value << YAML::Flow
            << YAML::BeginSeq << GridData.origin.x << GridData.origin.y << GridData.origin.z << YAML::EndSeq;
        out << YAML::Key << "cell_size" << YAML::Value << YAML::Flow
            << YAML::BeginSeq << GridData.voxelSize.x << GridData.voxelSize.y << GridData.voxelSize.z << YAML::EndSeq;
        out << YAML::Key << "voxel_count" << YAML::Value << VoxelCount;
        out << YAML::Key << "data" << YAML::Value << EncodeVoxelData(GridData.data);
        out << YAML::EndMap;

        out << YAML::EndMap;

        std::ofstream fout(filePath);
        if (!fout.is_open())
        {
            CB_CORE_ERROR("VoxelMeshAsset: Failed to save to {0}", filePath.string());
            return false;
        }

        fout << out.c_str();
        return true;
    }

    Ref<VoxelMeshAsset> VoxelMeshAsset::Load(const std::filesystem::path& filePath)
    {
        std::ifstream fin(filePath);
        if (!fin.is_open())
        {
            CB_CORE_ERROR("VoxelMeshAsset: Failed to open {0}", filePath.string());
            return nullptr;
        }

        YAML::Node data;
        try
        {
            data = YAML::Load(fin);
        }
        catch (const YAML::ParserException& e)
        {
            CB_CORE_ERROR("VoxelMeshAsset: YAML parse error in {0}: {1}", filePath.string(), e.what());
            return nullptr;
        }

        auto asset = CreateRef<VoxelMeshAsset>();
        asset->m_Path = filePath;

        // Source
        if (auto source = data["source"])
        {
            asset->SourceFilePath = source["path"].as<std::string>("");
            asset->SourceMeshUUID = UUID(source["uuid"].as<uint64_t>(0));
        }

        // Voxel settings
        if (auto settings = data["voxel_settings"])
        {
            asset->VoxelSettings.GridSize = settings["grid_size"].as<int>(32);
            asset->VoxelSettings.Solid = settings["solid"].as<bool>(true);
            asset->VoxelSettings.Padding = settings["padding"].as<float>(0.01f);
            asset->VoxelSettings.VoxelSize = settings["voxel_size"].as<float>(0.0f);
        }

        asset->ColorTextureUUID = UUID(data["color_texture"].as<uint64_t>(0));

        // Material slots
        if (auto slots = data["material_slots"])
        {
            for (const auto& slotNode : slots)
            {
                MaterialSlot slot;
                slot.Name = slotNode["name"].as<std::string>("");
                slot.MaterialUUID = UUID(slotNode["material"].as<uint64_t>(0));
                asset->MaterialSlots.push_back(slot);
            }
        }

        // Texture slots
        if (auto slots = data["texture_slots"])
        {
            for (const auto& slotNode : slots)
            {
                TextureSlot slot;
                slot.Name = slotNode["name"].as<std::string>("");
                slot.TextureUUID = UUID(slotNode["texture"].as<uint64_t>(0));
                asset->TextureSlots.push_back(slot);
            }
        }

        // Voxel grid
        if (auto grid = data["voxel_grid"])
        {
            auto dims = grid["dimensions"];
            if (dims && dims.IsSequence() && dims.size() == 3)
            {
                asset->GridData.size.x = dims[0].as<int>(0);
                asset->GridData.size.y = dims[1].as<int>(0);
                asset->GridData.size.z = dims[2].as<int>(0);
            }

            auto origin = grid["origin"];
            if (origin && origin.IsSequence() && origin.size() == 3)
            {
                asset->GridData.origin.x = origin[0].as<float>(0.0f);
                asset->GridData.origin.y = origin[1].as<float>(0.0f);
                asset->GridData.origin.z = origin[2].as<float>(0.0f);
            }

            auto cellSize = grid["cell_size"];
            if (cellSize && cellSize.IsSequence() && cellSize.size() == 3)
            {
                asset->GridData.voxelSize.x = cellSize[0].as<float>(0.0f);
                asset->GridData.voxelSize.y = cellSize[1].as<float>(0.0f);
                asset->GridData.voxelSize.z = cellSize[2].as<float>(0.0f);
            }

            asset->VoxelCount = grid["voxel_count"].as<uint64_t>(0);
            asset->GridData.totalVoxels = static_cast<uint64_t>(
                asset->GridData.size.x) * asset->GridData.size.y * asset->GridData.size.z;

            String encodedData = grid["data"].as<std::string>("");
            asset->GridData.data = DecodeVoxelData(encodedData);
        }

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
        VoxelSettings = reloaded->VoxelSettings;
        ColorTextureUUID = reloaded->ColorTextureUUID;
        GridData = reloaded->GridData;
        VoxelCount = reloaded->VoxelCount;
        MaterialSlots = reloaded->MaterialSlots;
        TextureSlots = reloaded->TextureSlots;
        return true;
    }
}
