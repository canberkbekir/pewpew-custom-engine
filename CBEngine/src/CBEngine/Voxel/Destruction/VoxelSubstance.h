#pragma once

#include "CBEngine/Math/CoreMath.h"
#include "CBEngine/Utils/VoxelMaterialType.h"

#include <cstdint>
#include <string>
#include <unordered_map>

namespace CB
{
	// =========================================================================
	// FractureBehavior — controls the visual cracking pattern and feel
	// =========================================================================
	enum class FractureBehavior : uint8_t
	{
		None = 0, // no visual stages, just disappears
		Chip = 1, // small chips fall off (stone)
		Crack = 2, // crack lines appear (wood)
		Shatter = 3, // explodes into many pieces (glass)
		Crumble = 4 // falls apart gradually (dirt, sandstone)
	};

	inline FractureBehavior FractureBehaviorFromString(const std::string& s)
	{
		if (s == "CHIP") return FractureBehavior::Chip;
		if (s == "CRACK") return FractureBehavior::Crack;
		if (s == "SHATTER") return FractureBehavior::Shatter;
		if (s == "CRUMBLE") return FractureBehavior::Crumble;
		return FractureBehavior::None;
	}

	// =========================================================================
	// VoxelDamageType — bitfield, allows combining types (Fire | Structural)
	// =========================================================================
	enum class VoxelDamageType : uint32_t
	{
		None = 0,
		Impact = 1 << 0, // physics collision force
		Explosion = 1 << 1, // radial blast pressure
		Slice = 1 << 2, // sharp cutting weapon
		Fire = 1 << 3, // heat / burn tick
		Acid = 1 << 4, // chemical dissolution
		Pressure = 1 << 5, // structural crush load
		Structural = 1 << 6 // support removed (cascade collapse)
	};

	// Hash for VoxelDamageType (needed for unordered_map key)
	struct VoxelDamageTypeHash
	{
		size_t operator()(VoxelDamageType t) const noexcept { return std::hash<uint32_t>()(static_cast<uint32_t>(t)); }
	};

	// =========================================================================
	// DamageTintConfig — per-damage-type visual tint configuration
	// =========================================================================
	struct DamageTintConfig
	{
		Vector3 Color = Vector3(1.0f); // target tint RGB (1,1,1 = no tint)
		float Intensity = 0.0f; // intensity gain per (effectiveDmg / maxHealth)
		int SpreadRadius = 0; // grid cells to spread, 0 = center only, max 4
		float SpreadFalloff = 0.5f; // intensity multiplier per cell distance
	};

	// =========================================================================
	// VoxelSubstanceProperties — physical destruction properties for one substance
	// Loaded from assets/config/voxel_substances.yaml by SubstanceRegistry
	// =========================================================================
	struct VoxelSubstanceProperties
	{
		// --- Physical ---
		float MassPerVoxel = 0.02f; // kg per voxel — fragment mass = VoxelCount * MassPerVoxel

		// --- Structural ---
		float Health = 100.0f;
		float Hardness = 50.0f; // 0-100; reduces Impact damage
		float ExplosionResistance = 0.5f; // 0-1 (1 = immune to explosions)
		float SliceResistance = 0.5f; // 0-1 (0 = cut like butter)
		float TensileStrength = 100.0f; // load capacity for structural integrity
		float ImpactThreshold = 10.0f; // min force (N) before any impact damage registers

		// --- Fracture ---
		FractureBehavior Fracture = FractureBehavior::Chip;
		float FractureThreshold = 0.3f; // health fraction at which cracking starts (0-1)
		int FragmentCount = 5;
		bool FragmentsHavePhysics = true;

		// --- Environmental ---
		bool Flammable = false;
		float IgnitionTemperature = 0.0f; // heat required to ignite
		float BurnDuration = 0.0f; // seconds, 0 = never burns out
		bool PropagatesDamage = false; // fire/explosion spreads to neighbors
		float DamageSpreadFactor = 0.5f; // damage multiplier when spreading

		// --- Soot ---
		float SootResistance = 0.5f; // 0 = absorbs soot easily, 1 = stays clean
		Vector3 SootColor = {0.2f, 0.18f, 0.15f}; // dark gray-brown soot tint

		// --- Cross-Entity Fire Spread ---
		float CrossEntitySpreadRadius = 0.5f; // world units — how far fire jumps to another mesh

		// --- Burn Stage Tints (visual progression when on fire) ---
		// Each color is a tint multiplier applied to the voxel's original palette color.
		// (1,1,1) = no tint, (0.1,0.08,0.05) = very dark.
		// Index 0 = Normal, 1 = Burning, 2 = Charred, 3 = Collapse
		static constexpr int kBurnStageCount = 4;
		Vector3 BurnStageTints[kBurnStageCount] = {
			{1.0f, 1.0f, 1.0f},     // Normal  — no tint (original color)
			{1.0f, 0.4f, 0.1f},     // Burning — warm orange darkening
			{0.15f, 0.08f, 0.03f},  // Charred — very dark brown
			{0.05f, 0.05f, 0.05f}   // Collapse — ash black
		};
		// Thresholds (fraction of BurnDuration elapsed) at which each stage begins
		// Stage 0 = [0, t[0]), Stage 1 = [t[0], t[1]), Stage 2 = [t[1], t[2]), Stage 3 = [t[2], 1.0]
		float BurnStageThresholds[3] = {0.1f, 0.5f, 0.85f};

		// --- Damage Tint (visual, per damage type) ---
		std::unordered_map<VoxelDamageType, DamageTintConfig, VoxelDamageTypeHash> DamageTints;
	};

	// =========================================================================
	// VoxelHealthState — per-voxel runtime state stored in the sparse damage map
	// =========================================================================
	struct VoxelHealthState
	{
		float CurrentHealth = 100.0f;
		float MaxHealth = 100.0f; // cached from substance at first damage
		uint8_t FractureStage = 0; // 0=intact 1=cracked 2=breaking 3=critical 4=destroyed
		bool Burning = false;
		bool Charred = false; // true once a burn has fully completed — prevents re-ignition
		float FireTimer = 0.0f; // counts down while burning
	};

	// =========================================================================
	// VoxelDamageEvent — describes a single damage impulse on a specific voxel
	// Queued by Lua scripts, physics callbacks, and engine systems
	// =========================================================================
	struct VoxelDamageEvent
	{
		glm::ivec3 GridCoord; // target voxel in grid-local integer space
		uint64_t EntityUUID = 0; // entity owning the voxel grid
		VoxelDamageType Type = VoxelDamageType::Impact;
		float RawAmount = 0.0f;
		glm::vec3 WorldOrigin = {}; // source world position (for impulse direction)
		glm::vec3 Direction = {}; // for Slice: blade direction
		uint64_t InstigatorUUID = 0; // who caused this (0 = no instigator)

		// When true, GridCoord is ignored and WorldHitPos is converted to grid
		// coords inside ProcessDamageQueue (where the grid is available).
		bool UseWorldPos = false;
		glm::vec3 WorldHitPos = {}; // world-space hit point (used when UseWorldPos=true)
	};
}