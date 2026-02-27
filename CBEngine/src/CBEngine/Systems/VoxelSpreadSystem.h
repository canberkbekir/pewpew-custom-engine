#pragma once

#include "ISystem.h"
#include "CBEngine/Core/TimeStep.h"
#include "CBEngine/Voxel/Destruction/VoxelDamageMap.h"
#include "CBEngine/Voxel/Destruction/VoxelSubstance.h"

#include <glm/glm.hpp>
#include <future>
#include <chrono>
#include <vector>
#include <unordered_map>

namespace CB
{
	class Scene;

	// Info about one destructible entity for cross-entity spread checks
	struct CrossEntityInfo
	{
		uint64_t EntityUUID;
		glm::vec3 AABBMin;
		glm::vec3 AABBMax;
		bool Flammable;
		float SootResistance;
		Vector3 SootColor;
		float BurnDuration;
	};

	// Shared context for cross-entity spread (const, built on main thread)
	struct CrossEntityContext
	{
		std::vector<CrossEntityInfo> Entities;
	};

	// Cross-entity ignition event produced by worker thread
	struct CrossEntityIgnition
	{
		uint64_t TargetEntityUUID;
		glm::vec3 WorldPos;
	};

	// Single burn that needs work this frame (flat, cache-friendly)
	struct BurnWorkItem
	{
		glm::ivec3 Coord;
		ActiveBurn Burn;   // snapshot copy of this burn's state
		bool NeedsTick;    // TickTimer expired
		bool NeedsSpread;  // SpreadTimer expired
		bool NeedsSoot;    // SootTimer expired
	};

	// Lightweight snapshot — flat work item vector instead of full burn map copy
	struct SpreadSnapshot
	{
		uint64_t EntityUUID;
		voxelizer::VoxelGrid Grid;              // deep copy only when grid generation changes
		std::vector<BurnWorkItem> WorkItems;    // flat contiguous array of burns needing work
		VoxelSubstanceProperties Substance;
		glm::mat4 WorldTransform = glm::mat4(1.0f);
	};

	// Results from one entity's spread tick (stride-sized, no full burn state)
	struct SpreadResult
	{
		uint64_t EntityUUID;
		std::vector<VoxelDamageEvent> TickDamage;        // burn tick damage to queue
		std::vector<glm::ivec3> NewIgnitions;            // neighbors to ignite
		VoxelTintMap TintUpdates;                        // tint changes to merge
		std::vector<CrossEntityIgnition> CrossIgnitions;  // fire jumping to other entities
	};

	class VoxelSpreadSystem
	{
	public:
		static void OnUpdate(Scene* scene, Timestep ts);
		static void Shutdown();
	private:
		// Main thread
		static void CollectWorkerResults();
		static void ApplyPendingResults(Scene* scene);
		static void AdvanceTimers(Scene* scene, float dt);
		static std::vector<SpreadSnapshot> BuildSnapshots(Scene* scene);
		static CrossEntityContext BuildCrossEntityContext(Scene* scene);

		// Worker thread (pure functions, no shared state)
		static std::vector<SpreadResult> ProcessSpreadTicks(
			std::vector<SpreadSnapshot> snapshots, CrossEntityContext crossCtx,
			std::vector<uint64_t>& outDeferred);
		static SpreadResult ProcessSingleEntity(const SpreadSnapshot& snap,
			const CrossEntityContext& crossCtx);

		// Async state
		static std::future<std::vector<SpreadResult>> s_WorkerFuture;
		static bool s_WorkerRunning;
		static std::vector<SpreadResult> s_PendingResults;
		static bool s_HasPendingResults;

		// Stride / scheduling
		static size_t s_StrideIndex;
		static std::vector<uint64_t> s_DeferredEntities;

		// Grid cache: avoids deep-copying grids every frame
		struct CachedGrid
		{
			uint32_t Generation = UINT32_MAX;
			voxelizer::VoxelGrid Grid;
		};
		static std::unordered_map<uint64_t, CachedGrid> s_GridCache;
	};

	// =========================================================================
	// Adapter registered in Scene::InitPhysics() at priority 176
	// =========================================================================
	class VoxelSpreadSystemAdapter : public ISystem
	{
	public:
		void OnUpdate(Scene* scene, Timestep ts) override { VoxelSpreadSystem::OnUpdate(scene, ts); }
		void Shutdown() override { VoxelSpreadSystem::Shutdown(); }
		const char* GetName() const override { return "VoxelSpreadSystem"; }
		int GetPriority() const override { return 176; }
	};
}
