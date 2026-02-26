#include "cbpch.h"
#include "AssetRegistry.h"
#include "CBEngine/Core/Log.h"

namespace CB
{
	// Normalize path for consistent comparison (use forward slashes, generic string)
	static std::string NormalizePath(const std::filesystem::path& path)
	{
		return path.lexically_normal().generic_string();
	}

	void AssetRegistry::Register(const AssetMetadata& metadata)
	{
		std::lock_guard<std::mutex> lock(m_Mutex);

		std::string normalizedPath = NormalizePath(metadata.FilePath);

		m_UUIDToMetadata[metadata.Handle] = metadata;
		m_PathToUUID[normalizedPath] = metadata.Handle;

		CB_CORE_TRACE("AssetRegistry: Registered {0} -> UUID {1}", normalizedPath,
		              static_cast<uint64_t>(metadata.Handle));

		// Update reverse dependencies
		for (UUID dependency : metadata.Dependencies) { m_ReverseDependencies[dependency].push_back(metadata.Handle); }
	}

	void AssetRegistry::Unregister(UUID uuid)
	{
		std::lock_guard<std::mutex> lock(m_Mutex);

		auto it = m_UUIDToMetadata.find(uuid);
		if (it != m_UUIDToMetadata.end()) {
			// Remove from path map
			m_PathToUUID.erase(NormalizePath(it->second.FilePath));

			// Remove from reverse dependencies
			for (UUID dependency : it->second.Dependencies) {
				auto& dependents = m_ReverseDependencies[dependency];
				dependents.erase(
					std::remove(dependents.begin(), dependents.end(), uuid),
					dependents.end()
				);
			}

			m_UUIDToMetadata.erase(it);
		}
	}

	const AssetMetadata* AssetRegistry::GetMetadata(UUID uuid) const
	{
		std::lock_guard<std::mutex> lock(m_Mutex);

		auto it = m_UUIDToMetadata.find(uuid);
		if (it != m_UUIDToMetadata.end())
			return &it->second;
		return nullptr;
	}

	const AssetMetadata* AssetRegistry::GetMetadataByPath(const std::filesystem::path& path) const
	{
		std::lock_guard<std::mutex> lock(m_Mutex);

		std::string normalizedPath = NormalizePath(path);
		auto it = m_PathToUUID.find(normalizedPath);
		if (it != m_PathToUUID.end()) {
			auto metaIt = m_UUIDToMetadata.find(it->second);
			if (metaIt != m_UUIDToMetadata.end())
				return &metaIt->second;
		}
		return nullptr;
	}

	UUID AssetRegistry::GetUUIDByPath(const std::filesystem::path& path) const
	{
		std::lock_guard<std::mutex> lock(m_Mutex);

		std::string normalizedPath = NormalizePath(path);
		auto it = m_PathToUUID.find(normalizedPath);
		if (it != m_PathToUUID.end())
			return it->second;
		return UUID::Invalid();
	}

	void AssetRegistry::UpdatePath(UUID uuid,const std::filesystem::path& newPath)
	{
		std::lock_guard<std::mutex> lock(m_Mutex);

		auto it = m_UUIDToMetadata.find(uuid);
		if (it != m_UUIDToMetadata.end()) {
			// Remove old path mapping
			m_PathToUUID.erase(NormalizePath(it->second.FilePath));

			// Update path
			it->second.FilePath = newPath;

			// Add new path mapping
			m_PathToUUID[NormalizePath(newPath)] = uuid;
		}
	}

	std::vector<UUID> AssetRegistry::GetDependents(UUID uuid) const
	{
		std::lock_guard<std::mutex> lock(m_Mutex);

		auto it = m_ReverseDependencies.find(uuid);
		if (it != m_ReverseDependencies.end())
			return it->second;
		return {};
	}

	void AssetRegistry::UpdateDependencies(UUID uuid,const std::vector<UUID>& deps)
	{
		std::lock_guard<std::mutex> lock(m_Mutex);

		auto it = m_UUIDToMetadata.find(uuid);
		if (it == m_UUIDToMetadata.end())
			return;

		// Remove old reverse dependencies
		for (UUID oldDep : it->second.Dependencies) {
			auto& dependents = m_ReverseDependencies[oldDep];
			dependents.erase(
				std::remove(dependents.begin(), dependents.end(), uuid),
				dependents.end()
			);
		}

		// Set new dependencies
		it->second.Dependencies = deps;

		// Add new reverse dependencies
		for (UUID newDep : deps) { m_ReverseDependencies[newDep].push_back(uuid); }
	}

	bool AssetRegistry::Contains(UUID uuid) const
	{
		std::lock_guard<std::mutex> lock(m_Mutex);
		return m_UUIDToMetadata.find(uuid) != m_UUIDToMetadata.end();
	}

	bool AssetRegistry::ContainsPath(const std::filesystem::path& path) const
	{
		std::lock_guard<std::mutex> lock(m_Mutex);
		return m_PathToUUID.find(NormalizePath(path)) != m_PathToUUID.end();
	}

	void AssetRegistry::Clear()
	{
		std::lock_guard<std::mutex> lock(m_Mutex);
		m_UUIDToMetadata.clear();
		m_PathToUUID.clear();
		m_ReverseDependencies.clear();
	}
}