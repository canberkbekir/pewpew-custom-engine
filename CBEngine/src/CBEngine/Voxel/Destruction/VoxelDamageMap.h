#pragma once

#include "VoxelSubstance.h"

#include <Voxelizer.h>
#include <glm/glm.hpp>
#include <unordered_map>
#include <vector>

namespace CB
{
    // =========================================================================
    // VoxelCoordHash — hash for glm::ivec3 used as unordered_map key
    // =========================================================================
    struct VoxelCoordHash
    {
        size_t operator()(const glm::ivec3& c) const noexcept
        {
            size_t h = std::hash<int>()(c.x);
            h ^= std::hash<int>()(c.y) + 0x9e3779b9u + (h << 6) + (h >> 2);
            h ^= std::hash<int>()(c.z) + 0x9e3779b9u + (h << 6) + (h >> 2);
            return h;
        }
    };

    // =========================================================================
    // VoxelDamageMap — sparse per-entity map of damaged voxel health states
    // Only voxels that have been damaged are stored; undamaged voxels are absent.
    // Keyed by grid-local integer coordinate.
    // =========================================================================
    using VoxelDamageMap = std::unordered_map<glm::ivec3, VoxelHealthState, VoxelCoordHash>;

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
    };
}
