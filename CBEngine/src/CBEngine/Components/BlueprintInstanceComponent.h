#pragma once

#include "CBEngine/Core/UUID.h"
#include "CBEngine/Core/String.h"

#include <yaml-cpp/yaml.h>

namespace CB
{
	struct BlueprintInstanceComponent
	{
		// Asset UUID of the source .blueprint file
		UUID BlueprintAssetUUID = UUID(0);

		// Relative path to .blueprint file (for display/fallback)
		String BlueprintPath;

		// Snapshot of the blueprint YAML at instantiation time (for change detection)
		String OriginalYAML;

		static constexpr auto YAMLKey = "BlueprintInstanceComponent";

		BlueprintInstanceComponent() = default;
		BlueprintInstanceComponent(const BlueprintInstanceComponent&) = default;

		void Serialize(YAML::Emitter& out) const
		{
			out << YAML::Key << "BlueprintAssetUUID" << YAML::Value << BlueprintAssetUUID;
			out << YAML::Key << "BlueprintPath" << YAML::Value << BlueprintPath;
			out << YAML::Key << "OriginalYAML" << YAML::Value << OriginalYAML;
		}

		void Deserialize(const YAML::Node& node)
		{
			if (node["BlueprintAssetUUID"])
				BlueprintAssetUUID = UUID(node["BlueprintAssetUUID"].as<uint64_t>());
			if (node["BlueprintPath"])
				BlueprintPath = node["BlueprintPath"].as<std::string>();
			if (node["OriginalYAML"])
				OriginalYAML = node["OriginalYAML"].as<std::string>();
		}
	};
}