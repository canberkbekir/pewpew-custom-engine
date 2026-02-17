#pragma once

#include "CBEngine/Math/CoreMath.h"
#include "CBEngine/Utils/YAMLHelpers.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>

namespace CB
{
	enum class ColliderShape : uint8_t
	{
		Box = 0,
		Sphere = 1,
		Capsule = 2,
		VoxelCompound = 3
	};

	struct ColliderComponent
	{
		ColliderShape Shape = ColliderShape::Box;

		// Box
		Vector3 HalfExtents = Vector3(0.5f);

		// Sphere
		float Radius = 0.5f;

		// Capsule
		float CapsuleRadius = 0.5f;
		float CapsuleHalfHeight = 0.5f;

		// Shared
		Vector3 Offset = Vector3(0.0f);
		bool IsTrigger = false;

		// Runtime-only (not serialized)
		JPH::RefConst<JPH::Shape> RuntimeShape;
		bool ShapeDirty = true;

		static constexpr auto YAMLKey = "ColliderComponent";

		ColliderComponent() = default;
		ColliderComponent(const ColliderComponent&) = default;

		void Serialize(YAML::Emitter& out) const
		{
			out << YAML::Key << "Shape" << YAML::Value << static_cast<int>(Shape);
			out << YAML::Key << "HalfExtents" << YAML::Value << HalfExtents;
			out << YAML::Key << "Radius" << YAML::Value << Radius;
			out << YAML::Key << "CapsuleRadius" << YAML::Value << CapsuleRadius;
			out << YAML::Key << "CapsuleHalfHeight" << YAML::Value << CapsuleHalfHeight;
			out << YAML::Key << "Offset" << YAML::Value << Offset;
			out << YAML::Key << "IsTrigger" << YAML::Value << IsTrigger;
		}

		void Deserialize(const YAML::Node& node)
		{
			Shape = static_cast<ColliderShape>(node["Shape"].as<int>());
			HalfExtents = node["HalfExtents"].as<glm::vec3>();
			Radius = node["Radius"].as<float>();
			CapsuleRadius = node["CapsuleRadius"].as<float>();
			CapsuleHalfHeight = node["CapsuleHalfHeight"].as<float>();
			Offset = node["Offset"].as<glm::vec3>();
			IsTrigger = node["IsTrigger"].as<bool>();
		}
	};
}
