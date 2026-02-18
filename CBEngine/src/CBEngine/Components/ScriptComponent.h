#pragma once

#include "CBEngine/Utils/YAMLHelpers.h"

#include <string>

namespace CB
{
	struct ScriptComponent
	{
		std::string ScriptPath; // e.g., "assets/scripts/player.lua"

		// Runtime-only (not serialized)
		bool ScriptLoaded = false;

		static constexpr auto YAMLKey = "ScriptComponent";

		ScriptComponent() = default;
		ScriptComponent(const ScriptComponent&) = default;

		void Serialize(YAML::Emitter& out) const
		{
			out << YAML::Key << "ScriptPath" << YAML::Value << ScriptPath;
		}

		void Deserialize(const YAML::Node& node)
		{
			if (node["ScriptPath"])
				ScriptPath = node["ScriptPath"].as<std::string>();
		}
	};
}
