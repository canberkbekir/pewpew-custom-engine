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
        VoxelDamageType Type = VoxelDamageType::Fire;  // Fire, Acid, etc.
        float Timer       = 0.0f;   // time remaining to burn (seconds)
        float SpreadTimer = 0.0f;   // cooldown until next spread attempt
        float TickTimer   = 0.0f;   // cooldown until next tick damage
    };

    using VoxelBurnMap = std::unordered_map<glm::ivec3, ActiveBurn, VoxelCoordHash>;

    // =========================================================================
    // EntityDestructionState — all runtime destruction data for one entity
    // Lives in VoxelDestructionSystem keyed by entity UUID (not a component).
    // =========================================================================
    struct EntityDestructionState
    {
        VoxelDamageMap              DamageMap;

        // Mutable working copy of the voxel grid — modified as voxels die.
        // Initialized (deep copied from vmesh asset) on first damage event.
        // Must be kept in sync: every coord removed from grid should be
        // erased from DamageMap too.
        bool                        GridInitialized = false;
        voxelizer::VoxelGrid        ModifiedGrid;
        std::vector<uint8_t>        ModifiedPaletteIndices;

        // Voxels that reached 0 HP this frame, pending removal from the grid
        std::vector<glm::ivec3>     PendingRemoval;

        // Tint map — per-voxel visual tint from damage effects
        VoxelTintMap TintMap;

        // Unified dirty flag: tint change OR voxel removal needs rebuild
        bool MeshDirty = false;

        // Active burns — voxels currently on fire/dissolving
        VoxelBurnMap ActiveBurns;
    };
}
