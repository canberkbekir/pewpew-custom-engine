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

		// Freeze constraints — lock individual axes at body creation time
		bool FreezePositionX = false;
		bool FreezePositionY = false;
		bool FreezePositionZ = false;
		bool FreezeRotationX = false;
		bool FreezeRotationY = false;
		bool FreezeRotationZ = false;

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
			out << YAML::Key << "FreezePositionX" << YAML::Value << FreezePositionX;
			out << YAML::Key << "FreezePositionY" << YAML::Value << FreezePositionY;
			out << YAML::Key << "FreezePositionZ" << YAML::Value << FreezePositionZ;
			out << YAML::Key << "FreezeRotationX" << YAML::Value << FreezeRotationX;
			out << YAML::Key << "FreezeRotationY" << YAML::Value << FreezeRotationY;
			out << YAML::Key << "FreezeRotationZ" << YAML::Value << FreezeRotationZ;
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
			if (node["FreezePositionX"]) FreezePositionX = node["FreezePositionX"].as<bool>();
			if (node["FreezePositionY"]) FreezePositionY = node["FreezePositionY"].as<bool>();
			if (node["FreezePositionZ"]) FreezePositionZ = node["FreezePositionZ"].as<bool>();
			if (node["FreezeRotationX"]) FreezeRotationX = node["FreezeRotationX"].as<bool>();
			if (node["FreezeRotationY"]) FreezeRotationY = node["FreezeRotationY"].as<bool>();
			if (node["FreezeRotationZ"]) FreezeRotationZ = node["FreezeRotationZ"].as<bool>();
		}
	};
}
