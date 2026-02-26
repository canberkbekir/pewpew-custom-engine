#pragma once

#include "Asset.h"
#include "CBEngine/Core/Core.h"
#include "CBEngine/Core/String.h"
#include "CBEngine/Core/Log.h"

#include <yaml-cpp/yaml.h>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace CB
{
	class BlueprintAsset : public Asset
	{
	public:
		BlueprintAsset() { m_Type = AssetType::Blueprint; }

		~BlueprintAsset() override = default;

		// Serialized YAML string of entities
		String YAMLData;

		// Display name (root entity tag)
		String RootEntityName;

		// Number of entities in hierarchy
		int EntityCount = 0;

		static Ref<BlueprintAsset> Load(const std::filesystem::path& path)
		{
			std::ifstream fin(path);
			if (!fin.is_open()) {
				CB_CORE_ERROR("BlueprintAsset::Load - Failed to open: {0}", path.string());
				return nullptr;
			}

			std::stringstream ss;
			ss << fin.rdbuf();
			String content = ss.str();

			auto asset = CreateRef<BlueprintAsset>();
			asset->m_Path = path;
			asset->YAMLData = content;

			// Parse basic metadata from YAML
			try {
				YAML::Node data = YAML::Load(content);
				if (data["Blueprint"])
					asset->RootEntityName = data["Blueprint"].as<std::string>();

				if (data["Entities"] && data["Entities"].IsSequence())
					asset->EntityCount = static_cast<int>(data["Entities"].size());
			}
			catch (const YAML::ParserException& e) {
				CB_CORE_ERROR("BlueprintAsset::Load - Failed to parse YAML: {0}", e.what());
				return nullptr;
			}

			return asset;
		}

		bool Save(const std::filesystem::path& path) const
		{
			std::ofstream fout(path);
			if (!fout.is_open()) {
				CB_CORE_ERROR("BlueprintAsset::Save - Failed to open: {0}", path.string());
				return false;
			}

			fout << YAMLData;

			if (fout.fail()) {
				CB_CORE_ERROR("BlueprintAsset::Save - Failed to write: {0}", path.string());
				return false;
			}

			CB_CORE_INFO("Saved blueprint: {0}", path.string());
			return true;
		}

		bool Reload() override
		{
			if (m_Path.empty())
				return false;

			auto reloaded = Load(m_Path);
			if (!reloaded)
				return false;

			YAMLData = reloaded->YAMLData;
			RootEntityName = reloaded->RootEntityName;
			EntityCount = reloaded->EntityCount;
			return true;
		}

		static AssetType GetStaticType() { return AssetType::Blueprint; }
	};
}