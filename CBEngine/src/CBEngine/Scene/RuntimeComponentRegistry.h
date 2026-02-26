#pragma once

#include "CBEngine/Core/Core.h"
#include <yaml-cpp/yaml.h>
#include <entt.hpp>

#include <functional>
#include <string>
#include <vector>

namespace CB
{
	class RuntimeComponentRegistry
	{
	public:
		using SerializeFn = std::function<void(YAML::Emitter&,entt::registry&,entt::entity)>;
		using DeserializeFn = std::function<void(const YAML::Node&,entt::registry&,entt::entity)>;
		using ResolveAssetsFn = std::function<void(entt::registry&,entt::entity)>; // may be nullptr

		struct Entry
		{
			std::string Key;
			SerializeFn Serialize;
			DeserializeFn Deserialize;
			ResolveAssetsFn ResolveAssets; // optional — nullptr if component has no asset refs
		};

		// Full registration (serialize + deserialize + optional asset resolution)
		static void Register(const std::string& key,
		                     SerializeFn serialize,
		                     DeserializeFn deserialize,
		                     ResolveAssetsFn resolveAssets = nullptr)
		{
			s_Entries.push_back({key, std::move(serialize), std::move(deserialize), std::move(resolveAssets)});
		}

		static void SerializeAll(YAML::Emitter& out,entt::registry& registry,entt::entity entity)
		{
			for (auto& entry : s_Entries)
				entry.Serialize(out, registry, entity);
		}

		static void DeserializeAll(const YAML::Node& entityNode,entt::registry& registry,entt::entity entity)
		{
			for (auto& entry : s_Entries) {
				auto node = entityNode[entry.Key];
				if (node)
					entry.Deserialize(node, registry, entity);
			}
		}

		static void ResolveAssetsAll(entt::registry& registry,entt::entity entity)
		{
			for (auto& entry : s_Entries) {
				if (entry.ResolveAssets)
					entry.ResolveAssets(registry, entity);
			}
		}

		static void Clear() { s_Entries.clear(); }

		static const std::vector<Entry>& GetEntries() { return s_Entries; }
	private:
		static inline std::vector<Entry> s_Entries;
	};
}