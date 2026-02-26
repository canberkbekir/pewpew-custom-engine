#pragma once

#include "CBEngine/Core/Core.h"
#include "CBEngine/Core/UUID.h"
#include "CBEngine/Math/CoreMath.h"
#include "CBEngine/Utils/YAMLHelpers.h"

#include <vector>

namespace YAML
{
	class Emitter;
	class Node;
}

namespace CB
{
	enum class ChainLinkType : uint8_t { Mesh = 0,Blueprint = 1 };

	struct HingeChainComponent
	{
		static constexpr auto YAMLKey = "HingeChainComponent";

		// ---- Link type ----
		ChainLinkType LinkType = ChainLinkType::Mesh;

		// ---- Chain structure ----
		int LinkCount = 5;
		float LinkSpacing = 1.0f;
		Vector3 ChainDirection = Vector3(0.0f, -1.0f, 0.0f); // local-space hang direction

		// ---- Per-link rendering (Mesh mode) ----
		UUID MeshUUID = UUID(0);
		UUID MaterialUUID = UUID(0);
		Vector3 LinkScale = Vector3(1.0f);

		// ---- Blueprint mode ----
		UUID BlueprintUUID = UUID(0);

		// ---- Per-link physics ----
		Vector3 ColliderHalfExtents = Vector3(0.4f, 0.4f, 0.4f);
		float LinkMass = 1.0f;
		float LinearDamping = 0.05f;
		float AngularDamping = 0.05f;

		// ---- Anchor ----
		// true  → first link's hinge attaches to world (static)
		// false → first link's hinge attaches to the chain root entity's rigid body
		bool AnchorFirstToWorld = true;

		// ---- Joint type ----
		// Ball = PointConstraint (free rotation in all axes, more realistic chain physics)
		// Hinge = 1-axis rotation (stiffer, use when you need a specific swing plane or motor)
		bool UseBallJoint = false;

		// ---- Hinge settings (applied to every link joint) ----
		Vector3 HingeAxis = Vector3(1.0f, 0.0f, 0.0f); // rotation axis in root/link local space

		bool UseLimits = false;
		float LimitsMin = -45.0f; // degrees
		float LimitsMax = 45.0f;
		float Bounciness = 0.0f;

		bool UseSpring = false;
		float SpringFrequency = 2.0f;
		float SpringDamping = 1.0f;

		bool UseMotor = false;
		float TargetVelocity = 0.0f; // degrees/second
		float MotorForce = 1000.0f; // Nm

		float BreakForce = 0.0f; // 0 = unbreakable
		float BreakTorque = 0.0f;

		bool EnableCollisionBetweenLinks = false;
		float MaxFrictionTorque = 0.0f;

		// ---- Runtime ----
		// Serialized so save/load preserves existing link entities without rebuilding.
		std::vector<UUID> LinkEntityUUIDs;

		HingeChainComponent() = default;
		HingeChainComponent(const HingeChainComponent&) = default;

		void Serialize(YAML::Emitter& out) const
		{
			out << YAML::Key << "LinkType" << YAML::Value << static_cast<int>(LinkType);
			out << YAML::Key << "LinkCount" << YAML::Value << LinkCount;
			out << YAML::Key << "LinkSpacing" << YAML::Value << LinkSpacing;
			out << YAML::Key << "ChainDirection" << YAML::Value << ChainDirection;
			out << YAML::Key << "MeshUUID" << YAML::Value << MeshUUID;
			out << YAML::Key << "MaterialUUID" << YAML::Value << MaterialUUID;
			out << YAML::Key << "LinkScale" << YAML::Value << LinkScale;
			out << YAML::Key << "BlueprintUUID" << YAML::Value << BlueprintUUID;
			out << YAML::Key << "ColliderHalfExtents" << YAML::Value << ColliderHalfExtents;
			out << YAML::Key << "LinkMass" << YAML::Value << LinkMass;
			out << YAML::Key << "LinearDamping" << YAML::Value << LinearDamping;
			out << YAML::Key << "AngularDamping" << YAML::Value << AngularDamping;
			out << YAML::Key << "AnchorFirstToWorld" << YAML::Value << AnchorFirstToWorld;
			out << YAML::Key << "UseBallJoint" << YAML::Value << UseBallJoint;
			out << YAML::Key << "HingeAxis" << YAML::Value << HingeAxis;
			out << YAML::Key << "UseLimits" << YAML::Value << UseLimits;
			out << YAML::Key << "LimitsMin" << YAML::Value << LimitsMin;
			out << YAML::Key << "LimitsMax" << YAML::Value << LimitsMax;
			out << YAML::Key << "Bounciness" << YAML::Value << Bounciness;
			out << YAML::Key << "UseSpring" << YAML::Value << UseSpring;
			out << YAML::Key << "SpringFrequency" << YAML::Value << SpringFrequency;
			out << YAML::Key << "SpringDamping" << YAML::Value << SpringDamping;
			out << YAML::Key << "UseMotor" << YAML::Value << UseMotor;
			out << YAML::Key << "TargetVelocity" << YAML::Value << TargetVelocity;
			out << YAML::Key << "MotorForce" << YAML::Value << MotorForce;
			out << YAML::Key << "BreakForce" << YAML::Value << BreakForce;
			out << YAML::Key << "BreakTorque" << YAML::Value << BreakTorque;
			out << YAML::Key << "EnableCollisionBetweenLinks" << YAML::Value << EnableCollisionBetweenLinks;
			out << YAML::Key << "MaxFrictionTorque" << YAML::Value << MaxFrictionTorque;

			out << YAML::Key << "LinkEntityUUIDs" << YAML::Value << YAML::BeginSeq;
			for (const UUID& id : LinkEntityUUIDs)
				out << id;
			out << YAML::EndSeq;
		}

		void Deserialize(const YAML::Node& node)
		{
			if (node["LinkType"]) {
				// New format: plain integer.  Old format: uint8_t was emitted as a
				// char-escaped string by YAML-cpp (e.g. "\x01").  Handle both.
				int v = node["LinkType"].as<int>(-1);
				if (v >= 0)
					LinkType = static_cast<ChainLinkType>(v);
				else {
					auto s = node["LinkType"].as<std::string>("");
					if (!s.empty())
						LinkType = static_cast<ChainLinkType>(static_cast<uint8_t>(s[0]));
				}
			}
			if (node["LinkCount"]) LinkCount = node["LinkCount"].as<int>();
			if (node["LinkSpacing"]) LinkSpacing = node["LinkSpacing"].as<float>();
			if (node["ChainDirection"]) ChainDirection = node["ChainDirection"].as<Vector3>();
			if (node["MeshUUID"]) MeshUUID = UUID(node["MeshUUID"].as<uint64_t>());
			if (node["MaterialUUID"]) MaterialUUID = UUID(node["MaterialUUID"].as<uint64_t>());
			if (node["LinkScale"]) LinkScale = node["LinkScale"].as<Vector3>();
			if (node["BlueprintUUID"]) BlueprintUUID = UUID(node["BlueprintUUID"].as<uint64_t>());
			if (node["ColliderHalfExtents"]) ColliderHalfExtents = node["ColliderHalfExtents"].as<Vector3>();
			if (node["LinkMass"]) LinkMass = node["LinkMass"].as<float>();
			if (node["LinearDamping"]) LinearDamping = node["LinearDamping"].as<float>();
			if (node["AngularDamping"]) AngularDamping = node["AngularDamping"].as<float>();
			if (node["AnchorFirstToWorld"]) AnchorFirstToWorld = node["AnchorFirstToWorld"].as<bool>();
			if (node["UseBallJoint"]) UseBallJoint = node["UseBallJoint"].as<bool>();
			if (node["HingeAxis"]) HingeAxis = node["HingeAxis"].as<Vector3>();
			if (node["UseLimits"]) UseLimits = node["UseLimits"].as<bool>();
			if (node["LimitsMin"]) LimitsMin = node["LimitsMin"].as<float>();
			if (node["LimitsMax"]) LimitsMax = node["LimitsMax"].as<float>();
			if (node["Bounciness"]) Bounciness = node["Bounciness"].as<float>();
			if (node["UseSpring"]) UseSpring = node["UseSpring"].as<bool>();
			if (node["SpringFrequency"]) SpringFrequency = node["SpringFrequency"].as<float>();
			if (node["SpringDamping"]) SpringDamping = node["SpringDamping"].as<float>();
			if (node["UseMotor"]) UseMotor = node["UseMotor"].as<bool>();
			if (node["TargetVelocity"]) TargetVelocity = node["TargetVelocity"].as<float>();
			if (node["MotorForce"]) MotorForce = node["MotorForce"].as<float>();
			if (node["BreakForce"]) BreakForce = node["BreakForce"].as<float>();
			if (node["BreakTorque"]) BreakTorque = node["BreakTorque"].as<float>();
			if (node["EnableCollisionBetweenLinks"])
				EnableCollisionBetweenLinks = node["EnableCollisionBetweenLinks"].as<bool>();
			if (node["MaxFrictionTorque"])
				MaxFrictionTorque = node["MaxFrictionTorque"].as<float>();

			LinkEntityUUIDs.clear();
			if (node["LinkEntityUUIDs"] && node["LinkEntityUUIDs"].IsSequence()) {
				for (const auto& uuidNode : node["LinkEntityUUIDs"])
					LinkEntityUUIDs.push_back(UUID(uuidNode.as<uint64_t>()));
			}
		}
	};
}