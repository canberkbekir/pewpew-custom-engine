#include "cbpch.h"
#include "AssetManager.h"
#include "ProcessedMeshAsset.h"
#include "VoxelTextureAsset.h"
#include "CBEngine/Core/Log.h"
#include "CBEngine/Renderer/Resources/Texture.h"
#include "CBEngine/Renderer/Resources/Mesh.h"
#include "CBEngine/Renderer/Resources/Shader.h"
#include "CBEngine/Renderer/Resources/Material.h"

#include <fstream>
#include <sstream>

namespace CB
{
    std::filesystem::path AssetManager::s_AssetDirectory;
    AssetRegistry AssetManager::s_Registry;
    std::unordered_map<UUID, Ref<Asset>> AssetManager::s_LoadedAssets;
    std::queue<UUID> AssetManager::s_ReloadQueue;
    std::mutex AssetManager::s_QueueMutex;
    std::mutex AssetManager::s_AssetsMutex;

    void AssetManager::Init(const std::filesystem::path& assetDirectory)
    {
        s_AssetDirectory = absolute(assetDirectory);

        CB_CORE_INFO("AssetManager: Working directory is {0}", std::filesystem::current_path().string());
        CB_CORE_INFO("AssetManager: Asset directory set to {0}", s_AssetDirectory.string());

        if (!exists(s_AssetDirectory))
        {
            CB_CORE_WARN("AssetManager: Asset directory does not exist, creating: {0}", s_AssetDirectory.string());
            create_directories(s_AssetDirectory);
        }

        // Scan for existing assets and .meta files
        ScanDirectory(s_AssetDirectory);

        CB_CORE_INFO("AssetManager initialized: {0} assets registered", s_Registry.GetAssetCount());
    }

    void AssetManager::Shutdown()
    {
        {
            std::lock_guard<std::mutex> lock(s_AssetsMutex);
            s_LoadedAssets.clear();
        }
        s_Registry.Clear();
        CB_CORE_INFO("AssetManager shutdown");
    }

    UUID AssetManager::ImportAsset(const std::filesystem::path& filePath)
    {
        std::filesystem::path fullPath = filePath;
        if (filePath.is_relative())
            fullPath = s_AssetDirectory / filePath;

        if (!exists(fullPath))
        {
            CB_CORE_ERROR("Asset file not found: {0}", fullPath.string());
            return UUID::Invalid();
        }

        // Check if already imported
        std::filesystem::path relativePath = relative(fullPath, s_AssetDirectory);
        UUID existingUUID = s_Registry.GetUUIDByPath(relativePath);
        if (existingUUID.IsValid())
            return existingUUID;

        // Create new UUID
        UUID uuid;

        // Determine asset type from extension
        std::string ext = fullPath.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), tolower);
        AssetType type = AssetTypeFromExtension(ext);

        if (type == AssetType::None)
        {
            CB_CORE_WARN("Unknown asset type for extension: {0}", ext);
            return UUID::Invalid();
        }

        // Create metadata
        AssetMetadata metadata;
        metadata.Handle = uuid;
        metadata.Type = type;
        metadata.FilePath = relativePath;

        // Register in registry
        s_Registry.Register(metadata);

        // Create .meta file
        CreateMetaFile(fullPath, uuid);

        CB_CORE_INFO("Imported asset: {0} (UUID: {1})", relativePath.string(), static_cast<uint64_t>(uuid));

        return uuid;
    }

    bool AssetManager::IsLoaded(UUID uuid)
    {
        std::lock_guard<std::mutex> lock(s_AssetsMutex);
        return s_LoadedAssets.find(uuid) != s_LoadedAssets.end();
    }

    void AssetManager::ReloadAsset(UUID uuid)
    {
        Ref<Asset> asset;
        {
            std::lock_guard<std::mutex> lock(s_AssetsMutex);
            auto it = s_LoadedAssets.find(uuid);
            if (it != s_LoadedAssets.end())
                asset = it->second;
        }

        if (asset)
        {
            if (asset->Reload())
            {
                CB_CORE_INFO("Reloaded asset: {0}", static_cast<uint64_t>(uuid));

                // Reload dependents
                auto dependents = s_Registry.GetDependents(uuid);
                for (UUID dependent : dependents)
                {
                    ReloadAsset(dependent);
                }
            }
            else
            {
                CB_CORE_ERROR("Failed to reload asset: {0}", static_cast<uint64_t>(uuid));
            }
        }
    }

    void AssetManager::ProcessReloadQueue()
    {
        std::lock_guard<std::mutex> lock(s_QueueMutex);

        while (!s_ReloadQueue.empty())
        {
            UUID uuid = s_ReloadQueue.front();
            s_ReloadQueue.pop();
            ReloadAsset(uuid);
        }
    }

    void AssetManager::QueueReload(UUID uuid)
    {
        std::lock_guard<std::mutex> lock(s_QueueMutex);
        s_ReloadQueue.push(uuid);
    }

    void AssetManager::RenameAsset(UUID uuid, const String& newName)
    {
        const AssetMetadata* metadata = s_Registry.GetMetadata(uuid);
        if (!metadata)
            return;

        std::filesystem::path oldPath = s_AssetDirectory / metadata->FilePath;
        std::filesystem::path newPath = oldPath.parent_path() / (newName + oldPath.extension().string());

        if (exists(newPath))
        {
            CB_CORE_ERROR("Cannot rename: file already exists: {0}", newPath.string());
            return;
        }

        // Rename file
        std::filesystem::rename(oldPath, newPath);

        // Rename .meta file
        std::filesystem::path oldMetaPath = GetMetaFilePath(oldPath);
        std::filesystem::path newMetaPath = GetMetaFilePath(newPath);
        if (exists(oldMetaPath))
        {
            std::filesystem::rename(oldMetaPath, newMetaPath);
        }

        // Update registry
        std::filesystem::path newRelativePath = relative(newPath, s_AssetDirectory);
        s_Registry.UpdatePath(uuid, newRelativePath);

        CB_CORE_INFO("Renamed asset to: {0}", newPath.string());
    }

    void AssetManager::MoveAsset(UUID uuid, const std::filesystem::path& newDirectory)
    {
        const AssetMetadata* metadata = s_Registry.GetMetadata(uuid);
        if (!metadata)
            return;

        std::filesystem::path oldPath = s_AssetDirectory / metadata->FilePath;
        std::filesystem::path newPath = newDirectory / oldPath.filename();

        if (exists(newPath))
        {
            CB_CORE_ERROR("Cannot move: file already exists: {0}", newPath.string());
            return;
        }

        // Move file
        std::filesystem::rename(oldPath, newPath);

        // Move .meta file
        std::filesystem::path oldMetaPath = GetMetaFilePath(oldPath);
        std::filesystem::path newMetaPath = GetMetaFilePath(newPath);
        if (exists(oldMetaPath))
        {
            std::filesystem::rename(oldMetaPath, newMetaPath);
        }

        // Update registry
        std::filesystem::path newRelativePath = relative(newPath, s_AssetDirectory);
        s_Registry.UpdatePath(uuid, newRelativePath);

        CB_CORE_INFO("Moved asset to: {0}", newPath.string());
    }

    void AssetManager::DeleteAsset(UUID uuid)
    {
        const AssetMetadata* metadata = s_Registry.GetMetadata(uuid);
        if (!metadata)
            return;

        std::filesystem::path fullPath = s_AssetDirectory / metadata->FilePath;

        // Remove from loaded assets
        {
            std::lock_guard<std::mutex> lock(s_AssetsMutex);
            s_LoadedAssets.erase(uuid);
        }

        // Unregister
        s_Registry.Unregister(uuid);

        // Delete files
        if (exists(fullPath))
        {
            std::filesystem::remove(fullPath);
        }

        std::filesystem::path metaPath = GetMetaFilePath(fullPath);
        if (exists(metaPath))
        {
            std::filesystem::remove(metaPath);
        }

        CB_CORE_INFO("Deleted asset: {0}", fullPath.string());
    }

    Ref<Asset> AssetManager::GetLoadedAsset(UUID uuid)
    {
        std::lock_guard<std::mutex> lock(s_AssetsMutex);
        auto it = s_LoadedAssets.find(uuid);
        if (it != s_LoadedAssets.end())
            return it->second;
        return nullptr;
    }

    void AssetManager::ScanDirectory(const std::filesystem::path& directory)
    {
        int filesFound = 0;
        int assetsImported = 0;
        int metaLoaded = 0;

        for (const auto& entry : std::filesystem::recursive_directory_iterator(directory))
        {
            if (entry.is_regular_file())
            {
                std::filesystem::path path = entry.path();

                // Skip .meta files in this scan
                if (path.extension() == ".meta")
                    continue;

                filesFound++;

                // Check for existing .meta file
                std::filesystem::path metaPath = GetMetaFilePath(path);
                if (exists(metaPath))
                {
                    LoadMetaFile(metaPath);
                    metaLoaded++;
                }
                else
                {
                    // Asset without .meta file - import it
                    std::string ext = path.extension().string();
                    std::transform(ext.begin(), ext.end(), ext.begin(), tolower);

                    if (AssetTypeFromExtension(ext) != AssetType::None)
                    {
                        ImportAsset(path);
                        assetsImported++;
                    }
                }
            }
        }

        CB_CORE_INFO("AssetManager: Scanned {0} files, loaded {1} existing .meta, imported {2} new assets",
                     filesFound, metaLoaded, assetsImported);
    }

    void AssetManager::LoadMetaFile(const std::filesystem::path& metaPath)
    {
        // Simple text format for now: UUID on first line, Type on second line
        // TODO: Replace with JSON parsing (nlohmann/json)
        std::ifstream file(metaPath);
        if (!file.is_open())
        {
            CB_CORE_ERROR("Failed to open meta file: {0}", metaPath.string());
            return;
        }

        std::string uuidStr, typeStr;
        if (!std::getline(file, uuidStr) || uuidStr.empty())
        {
            CB_CORE_ERROR("Invalid meta file (missing UUID): {0}", metaPath.string());
            return;
        }

        if (!std::getline(file, typeStr) || typeStr.empty())
        {
            CB_CORE_ERROR("Invalid meta file (missing type): {0}", metaPath.string());
            return;
        }

        uint64_t uuidValue = 0;
        try
        {
            uuidValue = std::stoull(uuidStr);
        }
        catch (const std::exception& e)
        {
            CB_CORE_ERROR("Invalid UUID in meta file {0}: {1}", metaPath.string(), e.what());
            return;
        }

        AssetType type = AssetTypeFromString(typeStr);
        if (type == AssetType::None)
        {
            CB_CORE_WARN("Unknown asset type in meta file: {0}", metaPath.string());
            return;
        }

        // Get asset path from meta path
        std::filesystem::path assetPath = metaPath;
        assetPath.replace_extension(); // Remove .meta

        AssetMetadata metadata;
        metadata.Handle = UUID(uuidValue);
        metadata.Type = type;
        metadata.FilePath = relative(assetPath, s_AssetDirectory);

        // Optional third line: deps:UUID1,UUID2,...
        std::string depsStr;
        if (std::getline(file, depsStr) && depsStr.substr(0, 5) == "deps:")
        {
            std::string depsList = depsStr.substr(5);
            std::stringstream ss(depsList);
            std::string token;
            while (std::getline(ss, token, ','))
            {
                if (!token.empty())
                {
                    try
                    {
                        uint64_t depUUID = std::stoull(token);
                        metadata.Dependencies.push_back(UUID(depUUID));
                    }
                    catch (...)
                    {
                    }
                }
            }
        }

        s_Registry.Register(metadata);
    }

    void AssetManager::CreateMetaFile(const std::filesystem::path& assetPath, UUID uuid)
    {
        std::filesystem::path metaPath = GetMetaFilePath(assetPath);

        std::string ext = assetPath.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), tolower);
        AssetType type = AssetTypeFromExtension(ext);

        // Simple text format for now
        // TODO: Replace with JSON serialization (nlohmann/json)
        std::ofstream file(metaPath);
        if (!file.is_open())
        {
            CB_CORE_ERROR("Failed to create meta file: {0}", metaPath.string());
            return;
        }

        file << uuid << "\n";
        file << AssetTypeToString(type) << "\n";

        if (file.fail())
        {
            CB_CORE_ERROR("Failed to write meta file: {0}", metaPath.string());
        }
    }

    std::filesystem::path AssetManager::GetMetaFilePath(const std::filesystem::path& assetPath)
    {
        return std::filesystem::path(assetPath.string() + ".meta");
    }

    Ref<Asset> AssetManager::LoadAssetInternal(const AssetMetadata& metadata)
    {
        std::filesystem::path fullPath = s_AssetDirectory / metadata.FilePath;

        if (!exists(fullPath))
        {
            CB_CORE_ERROR("Asset file not found: {0}", fullPath.string());
            return nullptr;
        }

        Ref<Asset> asset = nullptr;

        switch (metadata.Type)
        {
        case AssetType::Texture2D:
            {
                asset = Texture2D::Create(fullPath.string());
                break;
            }
        case AssetType::Mesh:
            {
                asset = Mesh::Load(fullPath.string());
                break;
            }
        case AssetType::Shader:
            {
                asset = Shader::Create(fullPath.string());
                break;
            }
        case AssetType::Material:
            {
                asset = Material::Load(fullPath.string());
                break;
            }
        case AssetType::Scene:
            {
                // Scene loading requires ECS implementation
                CB_CORE_WARN("Scene loading requires ECS implementation");
                break;
            }
        case AssetType::ProcessedMesh:
            {
                asset = ProcessedMeshAsset::Load(fullPath);
                break;
            }
        case AssetType::VoxelMesh:
            {
                asset = VoxelMeshAsset::Load(fullPath);
                break;
            }
        case AssetType::VoxelTexture:
            {
                asset = VoxelTextureAsset::Load(fullPath);
                break;
            }
        default:
            CB_CORE_ERROR("Unknown asset type: {0}", static_cast<int>(metadata.Type));
            break;
        }

        if (asset)
        {
            // Set the UUID on the loaded asset
            asset->m_UUID = metadata.Handle;
            asset->m_Type = metadata.Type;
            asset->m_Path = fullPath;
            CB_CORE_INFO("Loaded asset: {0} (UUID: {1})", fullPath.string(), static_cast<uint64_t>(metadata.Handle));
        }

        return asset;
    }
}
