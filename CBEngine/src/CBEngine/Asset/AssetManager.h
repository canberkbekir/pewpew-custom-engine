#pragma once

#include "Asset.h"
#include "AssetRegistry.h"
#include "CBEngine/Core/Core.h"

#include <queue>
#include <mutex>

namespace CB
{
	class AssetManager
	{
	public:
		static void Init(const std::filesystem::path& assetDirectory);
		static void Shutdown();

		static const std::filesystem::path& GetAssetDirectory() { return s_AssetDirectory; }

		// Import a new asset (creates .meta file if needed)
		static UUID ImportAsset(const std::filesystem::path& filePath);

		// Get asset by UUID (returns cached if already loaded)
		template<typename T>
		static Ref<T> GetAsset(UUID uuid);

		// Get asset by path (looks up UUID first)
		template<typename T>
		static Ref<T> GetAsset(const std::filesystem::path& path);

		// Check if asset is loaded
		static bool IsLoaded(UUID uuid);

		// Force reload
		static void ReloadAsset(UUID uuid);

		// Process pending hot reloads (call from main thread each frame)
		static void ProcessReloadQueue();

		// Queue asset for reload (thread-safe, called by file watcher)
		static void QueueReload(UUID uuid);

		// Asset operations
		static void RenameAsset(UUID uuid, const std::string& newName);
		static void MoveAsset(UUID uuid, const std::filesystem::path& newDirectory);
		static void DeleteAsset(UUID uuid);

		// Registry access
		static AssetRegistry& GetRegistry() { return s_Registry; }

		// Get loaded asset (nullptr if not loaded)
		static Ref<Asset> GetLoadedAsset(UUID uuid);

	private:
		static void ScanDirectory(const std::filesystem::path& directory);
		static void LoadMetaFile(const std::filesystem::path& metaPath);
		static void CreateMetaFile(const std::filesystem::path& assetPath, UUID uuid);
		static std::filesystem::path GetMetaFilePath(const std::filesystem::path& assetPath);

		static Ref<Asset> LoadAssetInternal(const AssetMetadata& metadata);

	private:
		static std::filesystem::path s_AssetDirectory;
		static AssetRegistry s_Registry;
		static std::unordered_map<UUID, Ref<Asset>> s_LoadedAssets;

		static std::queue<UUID> s_ReloadQueue;
		static std::mutex s_QueueMutex;
		static std::mutex s_AssetsMutex;  // Protects s_LoadedAssets
	};

	// Template implementations
	template<typename T>
	Ref<T> AssetManager::GetAsset(UUID uuid)
	{
		if (!uuid.IsValid())
			return nullptr;

		{
			// Check if already loaded (with lock)
			std::lock_guard<std::mutex> lock(s_AssetsMutex);
			auto it = s_LoadedAssets.find(uuid);
			if (it != s_LoadedAssets.end())
				return std::dynamic_pointer_cast<T>(it->second);
		}

		// Get metadata and load (outside lock to avoid blocking during load)
		const AssetMetadata* metadata = s_Registry.GetMetadata(uuid);
		if (!metadata)
			return nullptr;

		Ref<Asset> asset = LoadAssetInternal(*metadata);
		if (asset)
		{
			std::lock_guard<std::mutex> lock(s_AssetsMutex);
			// Double-check in case another thread loaded it
			auto it = s_LoadedAssets.find(uuid);
			if (it != s_LoadedAssets.end())
				return std::dynamic_pointer_cast<T>(it->second);

			s_LoadedAssets[uuid] = asset;
			return std::dynamic_pointer_cast<T>(asset);
		}

		return nullptr;
	}

	template<typename T>
	Ref<T> AssetManager::GetAsset(const std::filesystem::path& path)
	{
		UUID uuid = s_Registry.GetUUIDByPath(path);
		if (!uuid.IsValid())
		{
			// Try to import the asset
			uuid = ImportAsset(path);
		}
		return GetAsset<T>(uuid);
	}
}
