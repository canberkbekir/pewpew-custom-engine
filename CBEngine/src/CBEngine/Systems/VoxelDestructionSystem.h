#pragma once

#include "ISystem.h"
#include "CBEngine/Core/TimeStep.h"
#include "CBEngine/Voxel/Destruction/VoxelSubstance.h"
#include "CBEngine/Voxel/Destruction/VoxelDamageMap.h"

#include <glm/glm.hpp>
#include <unordered_map>
#include <vector>
#include <string>

namespace voxelizer
{
	struct VoxelGrid;
}

namespace CB
{
	class Scene;

	// =========================================================================
	// VoxelHit — returned by spatial query functions (used by Lua VoxelQuery API)
	// =========================================================================
	struct VoxelHit
	{
		uint64_t EntityUUID = 0;
		glm::vec3 WorldPos = {};
		glm::ivec3 GridCoord = {};
		std::string SubstanceName;
		float NormalizedHealth = 1.0f; // 1.0 = full, 0.0 = destroyed
	};

	// =========================================================================
	// VoxelDestructionSystem
	//
	// Manages per-voxel health tracking and drives the full destruction pipeline:
	//   1. Drain DamageEventQueue (filled by Lua scripts + physics callbacks)
	//   2. Apply substance-based damage resolution to health states
	//   3. Collect zero-HP voxels into PendingRemoval per entity
	//   4. Remove dead voxels from the entity's working grid copy
	//   5. Detect fragment splits (via VoxelSplitter::FindConnectedComponents)
	//   6. Rebuild single-piece mesh in-place OR destroy + spawn fragment entities
	//
	// Priority: 175 — runs after ScriptSystem (150), before DestructionSystem (200)
	// =========================================================================
	class VoxelDestructionSystem
	{
	public:
		// -----------------------------------------------------------------
		// Called by Lua (via VoxelDamage.Apply) and by PhysicsSystem for
		// automatic physics-impact damage. Thread-safe append — safe to call
		// from contact callbacks during physics step.
		// -----------------------------------------------------------------
		static void QueueDamage(const VoxelDamageEvent& event);

		// -----------------------------------------------------------------
		// Called once at startup (from Application) to load the substance DB.
		// path = absolute path to assets/config/voxel_substances.yaml
		// -----------------------------------------------------------------
		static void LoadSubstances(const std::string& path);

		// -----------------------------------------------------------------
		// Spatial queries — used by the Lua VoxelQuery API
		// -----------------------------------------------------------------
		static std::vector<VoxelHit> QuerySphere(Scene* scene,glm::vec3 origin,float radius);
		static std::vector<VoxelHit> QueryBox(Scene* scene,glm::vec3 center,glm::vec3 halfExtents);

		// -----------------------------------------------------------------
		// State reads — used by the Lua VoxelDamage API
		// -----------------------------------------------------------------
		static float GetNormalizedHealth(uint64_t entityUUID,glm::ivec3 gridCoord);
		static bool IsVoxelDestroyed(uint64_t entityUUID,glm::ivec3 gridCoord);
		static std::string GetSubstanceName(uint64_t entityUUID,glm::ivec3 gridCoord,Scene* scene);

		// -----------------------------------------------------------------
		// Main update — called by the system adapter each frame
		// -----------------------------------------------------------------
		static void OnUpdate(Scene* scene,Timestep ts);

		// -----------------------------------------------------------------
		// Get the modified voxel grid for an entity (for debug visualization).
		// Returns nullptr if the entity has no destruction state.
		// -----------------------------------------------------------------
		static const voxelizer::VoxelGrid* GetEntityGrid(uint64_t entityUUID);

		// -----------------------------------------------------------------
		// Cleanup — called from Scene::ShutdownPhysics
		// -----------------------------------------------------------------
		static void Shutdown();

		// -----------------------------------------------------------------
		// Access entity states map (for VoxelSpreadSystem to read/write burn data)
		// -----------------------------------------------------------------
		static std::unordered_map<uint64_t, EntityDestructionState>& GetEntityStates() { return s_EntityStates; }

		// -----------------------------------------------------------------
		// Called from on_destroy<DestructibleVoxelComponent> signal to
		// clean up s_EntityStates when an entity is destroyed externally.
		// -----------------------------------------------------------------
		static void OnEntityDestroyed(uint64_t uuid);

		// Mark an entity for tint-only mesh rebuild (used by VoxelSpreadSystem)
		static void MarkTintDirty(uint64_t entityUUID) { s_TintDirtyEntities.push_back(entityUUID); }
	private:
		static void ProcessDamageQueue(Scene* scene);
		static void ProcessPendingRemovals(Scene* scene);
		static void ProcessStructuralIntegrity(Scene* scene);

		static void RebuildOrFragment(Scene* scene,uint64_t entityUUID,
		                              EntityDestructionState& state);

		// Spawn a physics-simulated collapsed chunk from a set of voxel coords
		static void SpawnCollapsedChunk(Scene* scene,uint64_t sourceEntityUUID,
		                                const struct CollapseCluster& cluster,
		                                EntityDestructionState& state);

		// Compute effective damage given type, substance, and component multipliers
		static float ResolveEffectiveDamage(float raw,VoxelDamageType type,
		                                    const struct VoxelSubstanceProperties& sub,
		                                    float resistanceMultiplier);

		// Ensure state.ModifiedGrid is initialised from the entity's vmesh asset
		static bool EnsureGridInitialized(Scene* scene,uint64_t entityUUID,
		                                  EntityDestructionState& state);

		// Per-frame queues (drained at start of OnUpdate)
		static std::vector<VoxelDamageEvent> s_DamageQueue;

		// Per-entity destruction state (grid copy + damage map)
		static std::unordered_map<uint64_t, EntityDestructionState> s_EntityStates;

		// Entities that had voxels removed this frame — need structural check
		static std::vector<uint64_t> s_StructuralCheckQueue;

		// Entities needing tint-only mesh rebuild
		static std::vector<uint64_t> s_TintDirtyEntities;

		static void ProcessDirtyMeshes(Scene* scene);
	};

	// =========================================================================
	// Adapter registered in Scene::InitPhysics() at priority 175
	// =========================================================================
	class VoxelDestructionSystemAdapter : public ISystem
	{
	public:
		void OnUpdate(Scene* scene,Timestep ts) override { VoxelDestructionSystem::OnUpdate(scene, ts); }
		void Shutdown() override { VoxelDestructionSystem::Shutdown(); }
		const char* GetName() const override { return "VoxelDestructionSystem"; }
		int GetPriority() const override { return 175; }
	};
}