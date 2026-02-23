#pragma once

#include "CBEngine/Core/UUID.h"
#include "CBEngine/Utils/YAMLHelpers.h"

namespace CB
{
	struct AudioSourceComponent
	{
		UUID ClipHandle = UUID(0);   // AssetManager UUID for AudioClipAsset
		bool PlayOnCreate = true;
		bool Loop = false;
		float Volume = 1.0f;         // Override (-1 = use clip's default)
		float Pitch = 1.0f;
		bool Is3D = true;

		// Runtime — not serialized
		uint32_t RuntimeSourceID = 0;

		static constexpr auto YAMLKey = "AudioSourceComponent";

		AudioSourceComponent() = default;
		AudioSourceComponent(const AudioSourceComponent&) = default;

		void Serialize(YAML::Emitter& out) const
		{
			out << YAML::Key << "ClipHandle" << YAML::Value << static_cast<uint64_t>(ClipHandle);
			out << YAML::Key << "PlayOnCreate" << YAML::Value << PlayOnCreate;
			out << YAML::Key << "Loop" << YAML::Value << Loop;
			out << YAML::Key << "Volume" << YAML::Value << Volume;
			out << YAML::Key << "Pitch" << YAML::Value << Pitch;
			out << YAML::Key << "Is3D" << YAML::Value << Is3D;
		}

		void Deserialize(const YAML::Node& node)
		{
			if (node["ClipHandle"])
				ClipHandle = UUID(node["ClipHandle"].as<uint64_t>());
			if (node["PlayOnCreate"])
				PlayOnCreate = node["PlayOnCreate"].as<bool>();
			if (node["Loop"])
				Loop = node["Loop"].as<bool>();
			if (node["Volume"])
				Volume = node["Volume"].as<float>();
			if (node["Pitch"])
				Pitch = node["Pitch"].as<float>();
			if (node["Is3D"])
				Is3D = node["Is3D"].as<bool>();
		}
	};
}
