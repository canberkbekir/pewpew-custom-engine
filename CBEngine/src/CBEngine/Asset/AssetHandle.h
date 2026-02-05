#pragma once

#include "CBEngine/Core/UUID.h"
#include "CBEngine/Core/Core.h"
#include "AssetManager.h"

namespace CB
{
	// Lightweight handle that stores only UUID (8 bytes)
	// Resolves to actual asset on demand
	// Survives hot reload (asset pointer may change, but UUID stays same)
	template<typename T>
	class AssetHandle
	{
	public:
		AssetHandle()
			: m_UUID(UUID::Invalid())
		{
		}

		AssetHandle(UUID uuid)
			: m_UUID(uuid)
		{
		}

		AssetHandle(const AssetHandle&) = default;
		AssetHandle& operator=(const AssetHandle&) = default;

		bool IsValid() const { return m_UUID.IsValid(); }
		UUID GetUUID() const { return m_UUID; }

		// Resolve to actual asset (may trigger load)
		Ref<T> Get() const
		{
			if (!m_UUID.IsValid())
				return nullptr;
			return AssetManager::GetAsset<T>(m_UUID);
		}

		// Convenience operators
		Ref<T> operator->() const { return Get(); }
		T& operator*() const
		{
			Ref<T> asset = Get();
			CB_CORE_ASSERT(asset, "Dereferencing invalid or unloaded AssetHandle!");
			return *asset;
		}
		operator bool() const { return IsValid(); }

		bool operator==(const AssetHandle& other) const { return m_UUID == other.m_UUID; }
		bool operator!=(const AssetHandle& other) const { return m_UUID != other.m_UUID; }

		// Static factory
		static AssetHandle<T> Invalid() { return AssetHandle<T>(); }

	private:
		UUID m_UUID;
	};
}
