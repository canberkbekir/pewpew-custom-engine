#pragma once

#include "VoxelDamageMap.h"

namespace CB
{
	// =========================================================================
	// EntityDestructionState — all runtime destruction data for one entity
	// Lives in VoxelDestructionSystem keyed by entity UUID (not a component).
	// =========================================================================
	struct EntityDestructionState
	{
		VoxelDamageMap DamageMap;

		// Mutable working copy of the voxel grid — modified as voxels die.
		// Initialized (deep copied from vmesh asset) on first damage event.
		// Must be kept in sync: every coord removed from grid should be
		// erased from DamageMap too.
		bool GridInitialized = false;
		voxelizer::VoxelGrid ModifiedGrid;
		std::vector<uint8_t> ModifiedPaletteIndices;

		// Voxels that reached 0 HP this frame, pending removal from the grid
		std::vector<glm::ivec3> PendingRemoval;

		// Incremented each time voxels are removed from ModifiedGrid;
		// used by VoxelSpreadSystem grid cache to avoid redundant deep copies.
		uint32_t GridGeneration = 0;

		// Tint map — per-voxel visual tint from damage effects
		VoxelTintMap TintMap;

		// Unified dirty flag: tint change OR voxel removal needs rebuild
		bool MeshDirty = false;

		// Active burns — voxels currently on fire/dissolving
		VoxelBurnMap ActiveBurns;

		// Per-voxel heat map — sparse, only voxels that have received heat
		std::unordered_map<glm::ivec3, float, VoxelCoordHash> PerVoxelHeat;

		// Heat frontier — burning voxels with at least one non-burning filled neighbor
		// Rebuilt each frame by ConductEntityHeat; only these voxels do conduction work
		std::vector<glm::ivec3> HeatFrontier;

		// Grid generation at last full mesh rebuild — used to detect tint-only changes
		uint32_t LastMeshRebuildGeneration = UINT32_MAX;

		// Sparse filled coords list — maintained incrementally on voxel removal
		std::vector<glm::ivec3> FilledCoords;

		// Cached mapping: linear grid index → position in FilledCoords/palette
		// Rebuilt when CachedMapGeneration != GridGeneration
		std::unordered_map<uint64_t, size_t> CachedFilledIndexMap;
		uint32_t CachedMapGeneration = UINT32_MAX;

		// --- Deferred rebuild ---
		// Set by Impact/Explosion damage — forces immediate flush of PendingRemoval
		bool ForceImmediateRebuild = false;
		// Frames since last flush of PendingRemoval (counts up between flushes)
		uint32_t FramesSinceLastFlush = 0;
		// True if Impact/Explosion damage occurred since last FindCC — controls split detection
		bool HasInstantDamageSinceLastCC = false;
		// Fragment time-to-live: <0 = no TTL (default), >=0 = seconds remaining before despawn
		float FragmentTTL = -1.0f;
	};
}
