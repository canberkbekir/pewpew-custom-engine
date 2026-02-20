#pragma once

#include "CBEngine/Utils/YAMLHelpers.h"

namespace CB
{
	struct CameraComponent
	{
		bool Primary = true;
		float FOV = 45.0f;
		float NearClip = 0.1f;
		float FarClip = 100.0f;

		static constexpr auto YAMLKey = "CameraComponent";

		CameraComponent() = default;
		CameraComponent(const CameraComponent&) = default;

		void Serialize(YAML::Emitter& out) const
		{
			out << YAML::Key << "Primary" << YAML::Value << Primary;
			out << YAML::Key << "FOV" << YAML::Value << FOV;
			out << YAML::Key << "NearClip" << YAML::Value << NearClip;
			out << YAML::Key << "FarClip" << YAML::Value << FarClip;
		}

		void Deserialize(const YAML::Node& node)
		{
			Primary = node["Primary"].as<bool>();
			FOV = node["FOV"].as<float>();
			NearClip = node["NearClip"].as<float>();
			FarClip = node["FarClip"].as<float>();
		}
	};
}
