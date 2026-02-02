#include "pewpch.h"
#include "AssetManager.h"
#include "PewPew/Core/Log.h"

#include <fstream>
#include <sstream>

namespace PewPew
{
	std::filesystem::path AssetManager::s_AssetDirectory;
	AssetRegistry AssetManager::s_Registry;
	std::unordered_map<UUID, Ref<Asset>> AssetManager::s_LoadedAssets;
	std::queue<UUID> AssetManager::s_ReloadQueue;
	std::mutex AssetManager::s_QueueMutex;

	void AssetManager::Init(const std::filesystem::path& assetDirectory)
	{
		s_AssetDirectory = std::filesystem::absolute(assetDirectory);

		PEW_CORE_INFO("AssetManager: Working directory is {0}", std::filesystem::current_path().string());
		PEW_CORE_INFO("AssetManager: Asset directory set to {0}", s_AssetDirectory.string());

		if (!std::filesystem::exists(s_AssetDirectory))
		{
			PEW_CORE_WARN("AssetManager: Asset directory does not exist, creating: {0}", s_AssetDirectory.string());
			std::filesystem::create_directories(s_AssetDirectory);
		}

		// Scan for existing assets and .meta files
		ScanDirectory(s_AssetDirectory);

		PEW_CORE_INFO("AssetManager initialized: {0} assets registered", s_Registry.GetAssetCount());
	}

	void AssetManager::Shutdown()
	{
		s_LoadedAssets.clear();
		s_Registry.Clear();
		PEW_CORE_INFO("AssetManager shutdown");
	}

	UUID AssetManager::ImportAsset(const std::filesystem::path& filePath)
	{
		std::filesystem::path fullPath = filePath;
		if (filePath.is_relative())
			fullPath = s_AssetDirectory / filePath;

		if (!std::filesystem::exists(fullPath))
		{
			PEW_CORE_ERROR("Asset file not found: {0}", fullPath.string());
			return UUID::Invalid();
		}

		// Check if already imported
		std::filesystem::path relativePath = std::filesystem::relative(fullPath, s_AssetDirectory);
		UUID existingUUID = s_Registry.GetUUIDByPath(relativePath);
		if (existingUUID.IsValid())
			return existingUUID;

		// Create new UUID
		UUID uuid;

		// Determine asset type from extension
		std::string ext = fullPath.extension().string();
		std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
		AssetType type = AssetTypeFromExtension(ext);

		if (type == AssetType::None)
		{
			PEW_CORE_WARN("Unknown asset type for extension: {0}", ext);
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

		PEW_CORE_INFO("Imported asset: {0} (UUID: {1})", relativePath.string(), (uint64_t)uuid);

		return uuid;
	}

	bool AssetManager::IsLoaded(UUID uuid)
	{
		return s_LoadedAssets.find(uuid) != s_LoadedAssets.end();
	}

	void AssetManager::ReloadAsset(UUID uuid)
	{
		auto it = s_LoadedAssets.find(uuid);
		if (it != s_LoadedAssets.end())
		{
			if (it->second->Reload())
			{
				PEW_CORE_INFO("Reloaded asset: {0}", (uint64_t)uuid);

				// Reload dependents
				auto dependents = s_Registry.GetDependents(uuid);
				for (UUID dependent : dependents)
				{
					ReloadAsset(dependent);
				}
			}
			else
			{
				PEW_CORE_ERROR("Failed to reload asset: {0}", (uint64_t)uuid);
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

	void AssetManager::RenameAsset(UUID uuid, const std::string& newName)
	{
		const AssetMetadata* metadata = s_Registry.GetMetadata(uuid);
		if (!metadata)
			return;

		std::filesystem::path oldPath = s_AssetDirectory / metadata->FilePath;
		std::filesystem::path newPath = oldPath.parent_path() / (newName + oldPath.extension().string());

		if (std::filesystem::exists(newPath))
		{
			PEW_CORE_ERROR("Cannot rename: file already exists: {0}", newPath.string());
			return;
		}

		// Rename file
		std::filesystem::rename(oldPath, newPath);

		// Rename .meta file
		std::filesystem::path oldMetaPath = GetMetaFilePath(oldPath);
		std::filesystem::path newMetaPath = GetMetaFilePath(newPath);
		if (std::filesystem::exists(oldMetaPath))
		{
			std::filesystem::rename(oldMetaPath, newMetaPath);
		}

		// Update registry
		std::filesystem::path newRelativePath = std::filesystem::relative(newPath, s_AssetDirectory);
		s_Registry.UpdatePath(uuid, newRelativePath);

		PEW_CORE_INFO("Renamed asset to: {0}", newPath.string());
	}

	void AssetManager::MoveAsset(UUID uuid, const std::filesystem::path& newDirectory)
	{
		const AssetMetadata* metadata = s_Registry.GetMetadata(uuid);
		if (!metadata)
			return;

		std::filesystem::path oldPath = s_AssetDirectory / metadata->FilePath;
		std::filesystem::path newPath = newDirectory / oldPath.filename();

		if (std::filesystem::exists(newPath))
		{
			PEW_CORE_ERROR("Cannot move: file already exists: {0}", newPath.string());
			return;
		}

		// Move file
		std::filesystem::rename(oldPath, newPath);

		// Move .meta file
		std::filesystem::path oldMetaPath = GetMetaFilePath(oldPath);
		std::filesystem::path newMetaPath = GetMetaFilePath(newPath);
		if (std::filesystem::exists(oldMetaPath))
		{
			std::filesystem::rename(oldMetaPath, newMetaPath);
		}

		// Update registry
		std::filesystem::path newRelativePath = std::filesystem::relative(newPath, s_AssetDirectory);
		s_Registry.UpdatePath(uuid, newRelativePath);

		PEW_CORE_INFO("Moved asset to: {0}", newPath.string());
	}

	void AssetManager::DeleteAsset(UUID uuid)
	{
		const AssetMetadata* metadata = s_Registry.GetMetadata(uuid);
		if (!metadata)
			return;

		std::filesystem::path fullPath = s_AssetDirectory / metadata->FilePath;

		// Remove from loaded assets
		s_LoadedAssets.erase(uuid);

		// Unregister
		s_Registry.Unregister(uuid);

		// Delete files
		if (std::filesystem::exists(fullPath))
		{
			std::filesystem::remove(fullPath);
		}

		std::filesystem::path metaPath = GetMetaFilePath(fullPath);
		if (std::filesystem::exists(metaPath))
		{
			std::filesystem::remove(metaPath);
		}

		PEW_CORE_INFO("Deleted asset: {0}", fullPath.string());
	}

	Ref<Asset> AssetManager::GetLoadedAsset(UUID uuid)
	{
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
				if (std::filesystem::exists(metaPath))
				{
					LoadMetaFile(metaPath);
					metaLoaded++;
				}
				else
				{
					// Asset without .meta file - import it
					std::string ext = path.extension().string();
					std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

					if (AssetTypeFromExtension(ext) != AssetType::None)
					{
						ImportAsset(path);
						assetsImported++;
					}
				}
			}
		}

		PEW_CORE_INFO("AssetManager: Scanned {0} files, loaded {1} existing .meta, imported {2} new assets",
			filesFound, metaLoaded, assetsImported);
	}

	void AssetManager::LoadMetaFile(const std::filesystem::path& metaPath)
	{
		// Simple text format for now: UUID on first line, Type on second line
		// TODO: Replace with JSON parsing (nlohmann/json)
		std::ifstream file(metaPath);
		if (!file.is_open())
			return;

		std::string uuidStr, typeStr;
		std::getline(file, uuidStr);
		std::getline(file, typeStr);

		uint64_t uuidValue = std::stoull(uuidStr);
		AssetType type = AssetTypeFromString(typeStr);

		if (type == AssetType::None)
			return;

		// Get asset path from meta path
		std::filesystem::path assetPath = metaPath;
		assetPath.replace_extension();  // Remove .meta

		AssetMetadata metadata;
		metadata.Handle = UUID(uuidValue);
		metadata.Type = type;
		metadata.FilePath = std::filesystem::relative(assetPath, s_AssetDirectory);

		s_Registry.Register(metadata);
	}

	void AssetManager::CreateMetaFile(const std::filesystem::path& assetPath, UUID uuid)
	{
		std::filesystem::path metaPath = GetMetaFilePath(assetPath);

		std::string ext = assetPath.extension().string();
		std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
		AssetType type = AssetTypeFromExtension(ext);

		// Simple text format for now
		// TODO: Replace with JSON serialization (nlohmann/json)
		std::ofstream file(metaPath);
		if (file.is_open())
		{
			file << (uint64_t)uuid << "\n";
			file << AssetTypeToString(type) << "\n";
		}
	}

	std::filesystem::path AssetManager::GetMetaFilePath(const std::filesystem::path& assetPath)
	{
		return std::filesystem::path(assetPath.string() + ".meta");
	}

	Ref<Asset> AssetManager::LoadAssetInternal(const AssetMetadata& metadata)
	{
		// TODO: Implement asset loading based on type
		// This requires integration with existing Texture2D, Mesh, Shader classes
		// For now, return nullptr - user will implement loaders

		std::filesystem::path fullPath = s_AssetDirectory / metadata.FilePath;

		switch (metadata.Type)
		{
			case AssetType::Texture2D:
				// return Texture2D::Create(fullPath.string());
				PEW_CORE_WARN("Texture2D loading not yet integrated with AssetManager");
				break;
			case AssetType::Mesh:
				// return Mesh::Load(fullPath.string());
				PEW_CORE_WARN("Mesh loading not yet integrated with AssetManager");
				break;
			case AssetType::Shader:
				// return Shader::Create(fullPath.string());
				PEW_CORE_WARN("Shader loading not yet integrated with AssetManager");
				break;
			case AssetType::Material:
				PEW_CORE_WARN("Material loading not yet implemented");
				break;
			case AssetType::Scene:
				PEW_CORE_WARN("Scene loading not yet implemented");
				break;
			default:
				break;
		}

		return nullptr;
	}
}
