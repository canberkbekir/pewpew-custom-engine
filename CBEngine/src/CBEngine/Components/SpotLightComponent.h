#pragma once
#include "CBEngine/Math/CoreMath.h"
#include "CBEngine/Utils/YAMLHelpers.h"

namespace CB
{
	struct SpotLightComponent
	{
		bool Visible = true;

		Vector3 Color = {1.0f, 1.0f, 1.0f};
		float Intensity = 1.0f;
		float Range = 10.0f;
		float InnerAngleDegrees = 25.0f;
		float OuterAngleDegrees = 35.0f;

		static constexpr auto YAMLKey = "SpotLightComponent";

		SpotLightComponent() = default;

		void Serialize(YAML::Emitter& out) const
		{
			out << YAML::Key << "Visible" << YAML::Value << Visible;
			out << YAML::Key << "Color" << YAML::Value << Color;
			out << YAML::Key << "Intensity" << YAML::Value << Intensity;
			out << YAML::Key << "Range" << YAML::Value << Range;
			out << YAML::Key << "InnerAngleDegrees" << YAML::Value << InnerAngleDegrees;
			out << YAML::Key << "OuterAngleDegrees" << YAML::Value << OuterAngleDegrees;
		}

		void Deserialize(const YAML::Node& node)
		{
			Visible = node["Visible"].as<bool>();
			Color = node["Color"].as<glm::vec3>();
			Intensity = node["Intensity"].as<float>();
			Range = node["Range"].as<float>();
			InnerAngleDegrees = node["InnerAngleDegrees"].as<float>();
			OuterAngleDegrees = node["OuterAngleDegrees"].as<float>();
		}
	};
}
