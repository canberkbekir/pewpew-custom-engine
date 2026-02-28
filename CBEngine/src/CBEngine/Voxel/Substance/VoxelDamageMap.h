#pragma once

#include "VoxelSubstance.h"
#include "VoxelTintTypes.h"

#include <Voxelizer.h>
#include <glm/glm.hpp>
#include <unordered_map>
#include <vector>

namespace CB
{
	// =========================================================================
	// VoxelDamageMap — sparse per-entity map of damaged voxel health states
	// Only voxels that have been damaged are stored; undamaged voxels are absent.
	// Keyed by grid-local integer coordinate.
	// =========================================================================
	using VoxelDamageMap = std::unordered_map<glm::ivec3, VoxelHealthState, VoxelCoordHash>;

	// =========================================================================
	// ActiveBurn — per-voxel persistent burn/dissolve state
	// =========================================================================
	struct ActiveBurn
	{
		VoxelDamageType Type = VoxelDamageType::Fire; // Fire, Acid, etc.
		float Timer = 0.0f; // time remaining to burn (seconds)
		float SpreadTimer = 0.0f; // cooldown until next spread attempt
		float TickTimer = 0.0f; // cooldown until next tick damage
		float SootTimer = 0.0f; // cooldown until next soot application
		float HeatRampFactor = 0.0f; // 0->1 ramp via HeatAcceleration (octree heat output scaling)
	};

	using VoxelBurnMap = std::unordered_map<glm::ivec3, ActiveBurn, VoxelCoordHash>;
}