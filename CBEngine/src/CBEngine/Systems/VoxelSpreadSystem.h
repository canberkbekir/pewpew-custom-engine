#pragma once

#include "ISystem.h"
#include "CBEngine/Core/TimeStep.h"
#include "CBEngine/Voxel/Destruction/VoxelDamageMap.h"
#include "CBEngine/Voxel/Destruction/VoxelSubstance.h"

#include <glm/glm.hpp>
#include <future>
#include <chrono>
#include <vector>

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

	// Snapshot of one entity's burn state for worker thread
	struct SpreadSnapshot
	{
		uint64_t EntityUUID;
		voxelizer::VoxelGrid Grid; // deep copy (read-only on worker)
		VoxelBurnMap Burns; // deep copy
		VoxelSubstanceProperties Substance;
		float DeltaTime;
		glm::mat4 WorldTransform = glm::mat4(1.0f); // entity-to-world
	};

	// Results from one entity's spread tick
	struct SpreadResult
	{
		uint64_t EntityUUID;
		std::vector<VoxelDamageEvent> TickDamage; // burn tick damage to queue
		std::vector<glm::ivec3> NewIgnitions; // neighbors to ignite
		VoxelTintMap TintUpdates; // tint changes to merge
		VoxelBurnMap UpdatedBurns; // new burn state
		std::vector<glm::ivec3> Extinguished; // fires that expired
		std::vector<CrossEntityIgnition> CrossIgnitions; // fire jumping to other entities
	};

	class VoxelSpreadSystem
	{
	public:
		static void OnUpdate(Scene* scene,Timestep ts);
		static void Shutdown();
	private:
		// Main thread: apply previous frame's results, snapshot current state
		static void CollectWorkerResults();
		static void ApplyPendingResults(Scene* scene);
		static std::vector<SpreadSnapshot> BuildSnapshots(Scene* scene,float dt);
		static CrossEntityContext BuildCrossEntityContext(Scene* scene);

		// Worker thread: process spread ticks (pure function, no shared state)
		static std::vector<SpreadResult> ProcessSpreadTicks(
			std::vector<SpreadSnapshot> snapshots, CrossEntityContext crossCtx);
		static SpreadResult ProcessSingleEntity(const SpreadSnapshot& snap,
			const CrossEntityContext& crossCtx);

		// Async state
		static std::future<std::vector<SpreadResult>> s_WorkerFuture;
		static bool s_WorkerRunning;
		static std::vector<SpreadResult> s_PendingResults;
		static bool s_HasPendingResults;
	};

	// =========================================================================
	// Adapter registered in Scene::InitPhysics() at priority 176
	// =========================================================================
	class VoxelSpreadSystemAdapter : public ISystem
	{
	public:
		void OnUpdate(Scene* scene,Timestep ts) override { VoxelSpreadSystem::OnUpdate(scene, ts); }
		void Shutdown() override { VoxelSpreadSystem::Shutdown(); }
		const char* GetName() const override { return "VoxelSpreadSystem"; }
		int GetPriority() const override { return 176; }
	};
}
