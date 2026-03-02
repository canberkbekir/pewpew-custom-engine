#pragma once
#include "CBEngine/Math/CoreMath.h"
#include "CBEngine/Utils/YAMLHelpers.h"

namespace CB
{
	struct PointLightComponent
	{
		bool Visible = true;

		Vector3 Color = {1.0f, 1.0f, 1.0f};
		float Intensity = 1.0f;
		float Range = 10.0f;

		static constexpr auto YAMLKey = "PointLightComponent";

		PointLightComponent() = default;

		void Serialize(YAML::Emitter& out) const
		{
			out << YAML::Key << "Visible" << YAML::Value << Visible;
			out << YAML::Key << "Color" << YAML::Value << Color;
			out << YAML::Key << "Intensity" << YAML::Value << Intensity;
			out << YAML::Key << "Range" << YAML::Value << Range;
		}

		void Deserialize(const YAML::Node& node)
		{
			Visible = node["Visible"].as<bool>();
			Color = node["Color"].as<glm::vec3>();
			Intensity = node["Intensity"].as<float>();
			Range = node["Range"].as<float>();
		}
	};
}
