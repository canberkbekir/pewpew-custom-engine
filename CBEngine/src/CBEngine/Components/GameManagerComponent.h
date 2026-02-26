#pragma once

#include "CBEngine/Utils/YAMLHelpers.h"

namespace CB
{
	struct GameManagerComponent
	{
		bool IsGameManager = true; // Prevents entt from treating this as a tag type

		static constexpr auto YAMLKey = "GameManagerComponent";

		GameManagerComponent() = default;
		GameManagerComponent(const GameManagerComponent&) = default;

		void Serialize(YAML::Emitter& out) const
		{
			// Tag component - presence is the only data
		}

		void Deserialize(const YAML::Node& node)
		{
			// Nothing to deserialize
		}
	};
}