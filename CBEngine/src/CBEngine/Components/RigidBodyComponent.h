#pragma once

#include "CBEngine/Utils/YAMLHelpers.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>

namespace CB
{
	enum class BodyType : uint8_t
	{
		Static = 0,
		Dynamic = 1,
		Kinematic = 2
	};

	struct RigidBodyComponent
	{
		BodyType Type = BodyType::Dynamic;
		float Mass = 1.0f;
		float LinearDamping = 0.05f;
		float AngularDamping = 0.05f;
		float Friction = 0.5f;
		float Restitution = 0.3f;
		bool UseGravity = true;

		// Runtime-only (not serialized)
		JPH::BodyID RuntimeBodyID;
		bool BodyCreated = false;

		static constexpr auto YAMLKey = "RigidBodyComponent";

		RigidBodyComponent() = default;
		RigidBodyComponent(const RigidBodyComponent&) = default;

		void Serialize(YAML::Emitter& out) const
		{
			out << YAML::Key << "Type" << YAML::Value << static_cast<int>(Type);
			out << YAML::Key << "Mass" << YAML::Value << Mass;
			out << YAML::Key << "LinearDamping" << YAML::Value << LinearDamping;
			out << YAML::Key << "AngularDamping" << YAML::Value << AngularDamping;
			out << YAML::Key << "Friction" << YAML::Value << Friction;
			out << YAML::Key << "Restitution" << YAML::Value << Restitution;
			out << YAML::Key << "UseGravity" << YAML::Value << UseGravity;
		}

		void Deserialize(const YAML::Node& node)
		{
			Type = static_cast<BodyType>(node["Type"].as<int>());
			Mass = node["Mass"].as<float>();
			LinearDamping = node["LinearDamping"].as<float>();
			AngularDamping = node["AngularDamping"].as<float>();
			Friction = node["Friction"].as<float>();
			Restitution = node["Restitution"].as<float>();
			UseGravity = node["UseGravity"].as<bool>();
		}
	};
}
