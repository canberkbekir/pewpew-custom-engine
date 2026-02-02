#pragma once

#include "AssetMetadata.h"

#include <unordered_map>
#include <mutex>

namespace PewPew
{
	class AssetRegistry
	{
	public:
		void Register(const AssetMetadata& metadata);
		void Unregister(UUID uuid);

		const AssetMetadata* GetMetadata(UUID uuid) const;
		const AssetMetadata* GetMetadataByPath(const std::filesystem::path& path) const;
		UUID GetUUIDByPath(const std::filesystem::path& path) const;

		void UpdatePath(UUID uuid, const std::filesystem::path& newPath);

		const std::unordered_map<UUID, AssetMetadata>& GetAllAssets() const { return m_UUIDToMetadata; }
		size_t GetAssetCount() const { return m_UUIDToMetadata.size(); }

		std::vector<UUID> GetDependents(UUID uuid) const;

		bool Contains(UUID uuid) const;
		bool ContainsPath(const std::filesystem::path& path) const;

		void Clear();

	private:
		void BuildReverseDependencies();

	private:
		std::unordered_map<UUID, AssetMetadata> m_UUIDToMetadata;
		std::unordered_map<std::string, UUID> m_PathToUUID;  // string for easier comparison
		std::unordered_map<UUID, std::vector<UUID>> m_ReverseDependencies;

		mutable std::mutex m_Mutex;
	};
}
