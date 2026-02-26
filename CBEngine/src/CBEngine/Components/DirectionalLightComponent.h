#pragma once
#include "CBEngine/Math/CoreMath.h"
#include "CBEngine/Utils/YAMLHelpers.h"

namespace CB
{
	struct DirectionalLightComponent
	{
		bool Visible = true;

		Vector3 Color = {1.0f, 1.0f, 1.0f};
		float Intensity = 1.0f;

		bool CastShadows = false;
		float SourceAngleDegrees = 0.53f;

		static constexpr auto YAMLKey = "DirectionalLightComponent";

		DirectionalLightComponent() = default;

		void Serialize(YAML::Emitter& out) const
		{
			out << YAML::Key << "Visible" << YAML::Value << Visible;
			out << YAML::Key << "Color" << YAML::Value << Color;
			out << YAML::Key << "Intensity" << YAML::Value << Intensity;
			out << YAML::Key << "CastShadows" << YAML::Value << CastShadows;
			out << YAML::Key << "SourceAngleDegrees" << YAML::Value << SourceAngleDegrees;
		}

		void Deserialize(const YAML::Node& node)
		{
			Visible = node["Visible"].as<bool>();
			Color = node["Color"].as<glm::vec3>();
			Intensity = node["Intensity"].as<float>();
			CastShadows = node["CastShadows"].as<bool>();
			SourceAngleDegrees = node["SourceAngleDegrees"].as<float>();
		}
	};
}