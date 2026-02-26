#pragma once

#include "CBEngine/Math/CoreMath.h"
#include "CBEngine/Utils/VoxelMaterialType.h"
#include "CBEngine/Utils/YAMLHelpers.h"

namespace YAML
{
	class Emitter;
	class Node;
}

namespace CB
{
	// =========================================================================
	// DestructibleVoxelComponent
	//
	// Opt-in component for voxel entities. The VoxelDestructionSystem only
	// processes entities that have this component. Without it, a voxel object
	// is completely ignored by the damage pipeline.
	//
	// Place on any entity that also has VoxelRendererComponent.
	// =========================================================================
	struct DestructibleVoxelComponent
	{
		static constexpr auto YAMLKey = "DestructibleVoxelComponent";

		// --- Core toggle ---
		bool Enabled = true;

		// --- Physics auto-impact ---
		// When any Jolt rigid body contacts this entity's voxel surface,
		// the engine automatically queues an Impact damage event.
		bool ReceivePhysicsImpact = true;
		float PhysicsImpactThreshold = 10.0f; // min force (N) before damage registers
		float PhysicsImpactMultiplier = 1.0f; // scale applied on top of substance resistance

		// --- Structural integrity ---
		bool StructuralIntegrityEnabled = true;

		// BottomFace = bottom voxel row auto-treated as anchored to the ground
		// Manual     = only nearby VoxelAnchorComponent entities count as anchors
		// None       = disable collapse simulation (decorative rubble, etc.)
		enum class AnchorMode : int { BottomFace = 0,Manual = 1,None = 2 };
		AnchorMode Anchor = AnchorMode::BottomFace;

		// --- Substance override ---
		// Empty string = use per-voxel lookup from voxel registry + substances.yaml
		// Non-empty    = force all voxels on this entity to use this substance name
		//                (must match a key in voxel_substances.yaml, e.g. "Stone")
		std::string SubstanceOverride = "";

		// Quick per-object tuning without touching the yaml file.
		// > 1.0 = tougher,  < 1.0 = more fragile
		float HealthMultiplier = 1.0f;
		// > 1.0 = more resistant to all damage,  < 1.0 = less resistant
		float ResistanceMultiplier = 1.0f;

		// --- LOD / performance budget ---
		// Voxels beyond this distance from the camera skip fragment spawning.
		float FragmentSpawnDistance = 30.0f;
		// Voxels beyond this distance skip structural collapse checks.
		float StructuralCheckDistance = 50.0f;
		// Hard cap on simultaneous live fragment entities for this object.
		int MaxFragmentsPerObject = 20;
		// Max voxels this object can remove per frame (frame budget).
		int MaxDestructionsPerFrame = 30;

		// =====================================================================
		DestructibleVoxelComponent() = default;
		DestructibleVoxelComponent(const DestructibleVoxelComponent&) = default;

		void Serialize(YAML::Emitter& out) const
		{
			out << YAML::Key << "Enabled" << YAML::Value << Enabled;
			out << YAML::Key << "ReceivePhysicsImpact" << YAML::Value << ReceivePhysicsImpact;
			out << YAML::Key << "PhysicsImpactThreshold" << YAML::Value << PhysicsImpactThreshold;
			out << YAML::Key << "PhysicsImpactMultiplier" << YAML::Value << PhysicsImpactMultiplier;
			out << YAML::Key << "StructuralIntegrityEnabled" << YAML::Value << StructuralIntegrityEnabled;
			out << YAML::Key << "Anchor" << YAML::Value << static_cast<int>(Anchor);
			out << YAML::Key << "SubstanceOverride" << YAML::Value << SubstanceOverride;
			out << YAML::Key << "HealthMultiplier" << YAML::Value << HealthMultiplier;
			out << YAML::Key << "ResistanceMultiplier" << YAML::Value << ResistanceMultiplier;
			out << YAML::Key << "FragmentSpawnDistance" << YAML::Value << FragmentSpawnDistance;
			out << YAML::Key << "StructuralCheckDistance" << YAML::Value << StructuralCheckDistance;
			out << YAML::Key << "MaxFragmentsPerObject" << YAML::Value << MaxFragmentsPerObject;
			out << YAML::Key << "MaxDestructionsPerFrame" << YAML::Value << MaxDestructionsPerFrame;
		}

		void Deserialize(const YAML::Node& node)
		{
			if (node["Enabled"]) Enabled = node["Enabled"].as<bool>();
			if (node["ReceivePhysicsImpact"]) ReceivePhysicsImpact = node["ReceivePhysicsImpact"].as<bool>();
			if (node["PhysicsImpactThreshold"]) PhysicsImpactThreshold = node["PhysicsImpactThreshold"].as<float>();
			if (node["PhysicsImpactMultiplier"]) PhysicsImpactMultiplier = node["PhysicsImpactMultiplier"].as<float>();
			if (node["StructuralIntegrityEnabled"]) StructuralIntegrityEnabled = node["StructuralIntegrityEnabled"].as<
				bool>();
			if (node["Anchor"]) Anchor = static_cast<AnchorMode>(node["Anchor"].as<int>());
			if (node["SubstanceOverride"]) SubstanceOverride = node["SubstanceOverride"].as<std::string>();
			if (node["HealthMultiplier"]) HealthMultiplier = node["HealthMultiplier"].as<float>();
			if (node["ResistanceMultiplier"]) ResistanceMultiplier = node["ResistanceMultiplier"].as<float>();
			if (node["FragmentSpawnDistance"]) FragmentSpawnDistance = node["FragmentSpawnDistance"].as<float>();
			if (node["StructuralCheckDistance"]) StructuralCheckDistance = node["StructuralCheckDistance"].as<float>();
			if (node["MaxFragmentsPerObject"]) MaxFragmentsPerObject = node["MaxFragmentsPerObject"].as<int>();
			if (node["MaxDestructionsPerFrame"]) MaxDestructionsPerFrame = node["MaxDestructionsPerFrame"].as<int>();
		}
	};
}