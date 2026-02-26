#pragma once

#include "CBEngine/Math/CoreMath.h"
#include "CBEngine/Utils/VoxelMaterialType.h"

#include <cstdint>
#include <string>

namespace CB
{
    // =========================================================================
    // FractureBehavior — controls the visual cracking pattern and feel
    // =========================================================================
    enum class FractureBehavior : uint8_t
    {
        None    = 0,  // no visual stages, just disappears
        Chip    = 1,  // small chips fall off (stone)
        Crack   = 2,  // crack lines appear (wood)
        Shatter = 3,  // explodes into many pieces (glass)
        Crumble = 4   // falls apart gradually (dirt, sandstone)
    };

    inline FractureBehavior FractureBehaviorFromString(const std::string& s)
    {
        if (s == "CHIP")    return FractureBehavior::Chip;
        if (s == "CRACK")   return FractureBehavior::Crack;
        if (s == "SHATTER") return FractureBehavior::Shatter;
        if (s == "CRUMBLE") return FractureBehavior::Crumble;
        return FractureBehavior::None;
    }

    // =========================================================================
    // VoxelDamageType — bitfield, allows combining types (Fire | Structural)
    // =========================================================================
    enum class VoxelDamageType : uint32_t
    {
        None       = 0,
        Impact     = 1 << 0,  // physics collision force
        Explosion  = 1 << 1,  // radial blast pressure
        Slice      = 1 << 2,  // sharp cutting weapon
        Fire       = 1 << 3,  // heat / burn tick
        Acid       = 1 << 4,  // chemical dissolution
        Pressure   = 1 << 5,  // structural crush load
        Structural = 1 << 6   // support removed (cascade collapse)
    };

    // =========================================================================
    // VoxelSubstanceProperties — physical destruction properties for one substance
    // Loaded from assets/config/voxel_substances.yaml by VoxelSubstanceDatabase
    // =========================================================================
    struct VoxelSubstanceProperties
    {
        // --- Structural ---
        float Health              = 100.0f;
        float Hardness            = 50.0f;   // 0-100; reduces Impact damage
        float ExplosionResistance = 0.5f;    // 0-1 (1 = immune to explosions)
        float SliceResistance     = 0.5f;    // 0-1 (0 = cut like butter)
        float TensileStrength     = 100.0f;  // load capacity for structural integrity
        float ImpactThreshold     = 10.0f;   // min force (N) before any impact damage registers

        // --- Fracture ---
        FractureBehavior Fracture        = FractureBehavior::Chip;
        float            FractureThreshold = 0.3f;  // health fraction at which cracking starts (0-1)
        int              FragmentCount     = 5;
        bool             FragmentsHavePhysics = true;

        // --- Environmental ---
        bool  Flammable           = false;
        float IgnitionTemperature = 0.0f;    // heat required to ignite
        float BurnDuration        = 0.0f;    // seconds, 0 = never burns out
        bool  PropagatesDamage    = false;   // fire/explosion spreads to neighbors
        float DamageSpreadFactor  = 0.5f;    // damage multiplier when spreading
    };

    // =========================================================================
    // VoxelHealthState — per-voxel runtime state stored in the sparse damage map
    // =========================================================================
    struct VoxelHealthState
    {
        float   CurrentHealth = 100.0f;
        float   MaxHealth     = 100.0f;  // cached from substance at first damage
        uint8_t FractureStage = 0;       // 0=intact 1=cracked 2=breaking 3=critical 4=destroyed
        bool    Burning       = false;
        float   FireTimer     = 0.0f;    // counts down while burning
    };

    // =========================================================================
    // VoxelDamageEvent — describes a single damage impulse on a specific voxel
    // Queued by Lua scripts, physics callbacks, and engine systems
    // =========================================================================
    struct VoxelDamageEvent
    {
        glm::ivec3      GridCoord;          // target voxel in grid-local integer space
        uint64_t        EntityUUID = 0;     // entity owning the voxel grid
        VoxelDamageType Type       = VoxelDamageType::Impact;
        float           RawAmount  = 0.0f;
        glm::vec3       WorldOrigin = {};   // source world position (for impulse direction)
        glm::vec3       Direction   = {};   // for Slice: blade direction
        uint64_t        InstigatorUUID = 0; // who caused this (0 = no instigator)

        // When true, GridCoord is ignored and WorldHitPos is converted to grid
        // coords inside ProcessDamageQueue (where the grid is available).
        bool            UseWorldPos = false;
        glm::vec3       WorldHitPos = {};   // world-space hit point (used when UseWorldPos=true)
    };
}
