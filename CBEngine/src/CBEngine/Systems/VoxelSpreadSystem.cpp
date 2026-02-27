#include "cbpch.h"
#include "VoxelSpreadSystem.h"
#include "VoxelDestructionSystem.h"

#include "CBEngine/Scene/Scene.h"
#include "CBEngine/Scene/Entity.h"
#include "CBEngine/Components/TransformComponent.h"
#include "CBEngine/Components/DestructibleVoxelComponent.h"
#include "CBEngine/Components/VoxelRendererComponent.h"
#include "CBEngine/Asset/ProcessedMeshAsset.h"
#include "CBEngine/Asset/AssetManager.h"
#include "CBEngine/Voxel/Destruction/SubstanceRegistry.h"
#include "CBEngine/Voxel/Destruction/VoxelTintTypes.h"

#include "CBEngine/Debug/Instrumentor.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <chrono>

namespace CB
{
	// Static storage
	std::future<std::vector<SpreadResult>> VoxelSpreadSystem::s_WorkerFuture;
	bool VoxelSpreadSystem::s_WorkerRunning = false;
	std::vector<SpreadResult> VoxelSpreadSystem::s_PendingResults;
	bool VoxelSpreadSystem::s_HasPendingResults = false;

	size_t VoxelSpreadSystem::s_StrideIndex = 0;
	std::vector<uint64_t> VoxelSpreadSystem::s_DeferredEntities;
	std::unordered_map<uint64_t, VoxelSpreadSystem::CachedGrid> VoxelSpreadSystem::s_GridCache;

	// =========================================================================
	// OnUpdate
	// =========================================================================
	void VoxelSpreadSystem::OnUpdate(Scene* scene, Timestep ts)
	{
		CB_PROFILE_SCOPE("VoxelSpreadSystem::OnUpdate");
		CollectWorkerResults();

		if (s_HasPendingResults)
			ApplyPendingResults(scene);

		if (!s_WorkerRunning) {
			float dt = glm::min(static_cast<float>(ts), 1.0f / 30.0f);
			AdvanceTimers(scene, dt);
			auto snapshots = BuildSnapshots(scene);
			if (!snapshots.empty()) {
				auto crossCtx = BuildCrossEntityContext(scene);
				s_WorkerRunning = true;
				s_WorkerFuture = std::async(std::launch::async,
					[snaps = std::move(snapshots), ctx = std::move(crossCtx)]() mutable {
						std::vector<uint64_t> deferred;
						auto results = ProcessSpreadTicks(std::move(snaps), std::move(ctx), deferred);
						// Store deferred for main thread to pick up (piggyback on results)
						// We use a static here since the main thread reads it after future.get()
						s_DeferredEntities = std::move(deferred);
						return results;
					});
			}
		}
	}

	// =========================================================================
	// CollectWorkerResults
	// =========================================================================
	void VoxelSpreadSystem::CollectWorkerResults()
	{
		if (!s_WorkerRunning) return;
		if (s_WorkerFuture.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready)
			return;

		s_PendingResults = s_WorkerFuture.get();
		s_HasPendingResults = true;
		s_WorkerRunning = false;
	}

	// =========================================================================
	// AdvanceTimers — main thread, O(N) float math for all burns
	// =========================================================================
	void VoxelSpreadSystem::AdvanceTimers(Scene* scene, float dt)
	{
		CB_PROFILE_SCOPE("VoxelSpreadSystem::AdvanceTimers");
		auto& entityStates = VoxelDestructionSystem::GetEntityStates();

		for (auto& [uuid, state] : entityStates) {
			if (state.ActiveBurns.empty()) continue;

			// Look up substance for burn stage tint interpolation
			Entity entity = scene->GetEntityByUUID(UUID(uuid));
			VoxelSubstanceProperties sub{};
			if (entity && entity.HasComponent<DestructibleVoxelComponent>()) {
				auto& dvComp = entity.GetComponent<DestructibleVoxelComponent>();
				const SubstanceID& subID = dvComp.SubstanceOverride.empty()
					? std::string(kDefaultSubstanceID) : dvComp.SubstanceOverride;
				sub = SubstanceRegistry::Get(subID);
			}

			bool tintChanged = false;
			std::vector<glm::ivec3> expired;
			for (auto& [coord, burn] : state.ActiveBurns) {
				burn.Timer -= dt;
				burn.TickTimer -= dt;
				burn.SpreadTimer -= dt;
				burn.SootTimer -= dt;

				if (burn.Timer <= 0.0f) {
					expired.push_back(coord);
					// Lock tint to final charred stage so it doesn't revert
					auto& tint = state.TintMap[coord];
					tint.Color = sub.BurnStageTints[3];
					tint.Intensity = 1.0f;
					tintChanged = true;
					continue;
				}

				// Burn stage tint — computed on main thread every frame for smooth visuals
				float burnProgress = 1.0f - (burn.Timer / glm::max(sub.BurnDuration, 0.001f));
				burnProgress = glm::clamp(burnProgress, 0.0f, 1.0f);

				int stageIdx = 0;
				for (int s = 2; s >= 0; --s) {
					if (burnProgress >= sub.BurnStageThresholds[s]) {
						stageIdx = s + 1;
						break;
					}
				}

				float stageStart = (stageIdx > 0) ? sub.BurnStageThresholds[stageIdx - 1] : 0.0f;
				float stageEnd = (stageIdx < 3) ? sub.BurnStageThresholds[stageIdx] : 1.0f;
				float t = glm::clamp(
					(burnProgress - stageStart) / glm::max(stageEnd - stageStart, 0.001f), 0.0f, 1.0f);
				int nextStage = glm::min(stageIdx + 1, 3);
				Vector3 tintColor = glm::mix(
					sub.BurnStageTints[stageIdx],
					sub.BurnStageTints[nextStage], t);

				auto& tint = state.TintMap[coord];
				tint.Color = tintColor;
				tint.Intensity = 1.0f;
				tintChanged = true;
			}

			if (tintChanged) {
				state.MeshDirty = true;
				VoxelDestructionSystem::MarkTintDirty(uuid);
			}

			// Remove expired burns and mark as charred
			for (const auto& c : expired) {
				state.ActiveBurns.erase(c);
				auto& health = state.DamageMap[c];
				health.Burning = false;
				health.Charred = true;
				health.FireTimer = 0.0f;
			}
		}
	}

	// =========================================================================
	// BuildSnapshots — lightweight, stride-filtered
	// =========================================================================
	std::vector<SpreadSnapshot> VoxelSpreadSystem::BuildSnapshots(Scene* scene)
	{
		CB_PROFILE_SCOPE("VoxelSpreadSystem::BuildSnapshots");
		std::vector<SpreadSnapshot> snapshots;
		auto& entityStates = VoxelDestructionSystem::GetEntityStates();

		// Compute total burn count for stride calculation
		int totalBurns = 0;
		for (auto& [uuid, state] : entityStates)
			totalBurns += static_cast<int>(state.ActiveBurns.size());

		int strideCount = std::max(1, totalBurns / 2000);

		// Prioritize deferred entities by processing them first
		auto deferredSet = std::unordered_set<uint64_t>(
			s_DeferredEntities.begin(), s_DeferredEntities.end());
		s_DeferredEntities.clear();

		for (auto& [uuid, state] : entityStates) {
			if (state.ActiveBurns.empty() || !state.GridInitialized)
				continue;

			Entity entity = scene->GetEntityByUUID(UUID(uuid));
			if (!entity || !entity.HasComponent<DestructibleVoxelComponent>())
				continue;

			auto& dvComp = entity.GetComponent<DestructibleVoxelComponent>();
			const SubstanceID& subID = dvComp.SubstanceOverride.empty()
				? std::string(kDefaultSubstanceID) : dvComp.SubstanceOverride;

			SpreadSnapshot snap;
			snap.EntityUUID = uuid;
			snap.Substance = SubstanceRegistry::Get(subID);

			// Grid: use cache — only deep copy when generation changes
			auto& cached = s_GridCache[uuid];
			if (cached.Generation != state.GridGeneration) {
				cached.Grid = state.ModifiedGrid; // deep copy
				cached.Generation = state.GridGeneration;
			}
			snap.Grid = cached.Grid;

			if (entity.HasComponent<TransformComponent>())
				snap.WorldTransform = entity.GetComponent<TransformComponent>().GetTransform();

			// Build work items: only burns that have expired timers AND match stride
			for (auto& [coord, burn] : state.ActiveBurns) {
				bool needsTick = burn.TickTimer <= 0.0f;
				bool needsSpread = burn.SpreadTimer <= 0.0f;
				bool needsSoot = burn.SootTimer <= 0.0f;

				if (!needsTick && !needsSpread && !needsSoot)
					continue;

				// Stride filter: deferred entities bypass stride (get priority)
				if (!deferredSet.count(uuid)) {
					size_t h = VoxelCoordHash()(coord);
					if ((h % strideCount) != (s_StrideIndex % strideCount))
						continue;
				}

				snap.WorkItems.push_back({ coord, burn, needsTick, needsSpread, needsSoot });

				// Reset timers on main thread (worker won't touch timers)
				if (needsTick) burn.TickTimer = 0.2f;
				if (needsSpread) burn.SpreadTimer = 0.25f;
				if (needsSoot) burn.SootTimer = 0.15f;
			}

			if (!snap.WorkItems.empty())
				snapshots.push_back(std::move(snap));
		}

		s_StrideIndex++;
		return snapshots;
	}

	// =========================================================================
	// BuildCrossEntityContext
	// =========================================================================
	CrossEntityContext VoxelSpreadSystem::BuildCrossEntityContext(Scene* scene)
	{
		CrossEntityContext ctx;
		auto& entityStates = VoxelDestructionSystem::GetEntityStates();

		// Iterate ALL scene entities with DestructibleVoxelComponent (not just those with state)
		auto view = scene->GetEntitiesWith<DestructibleVoxelComponent>();
		for (auto enttID : view) {
			Entity entity(enttID, scene);
			uint64_t uuid = static_cast<uint64_t>(entity.GetUUID());

			auto& dvComp = entity.GetComponent<DestructibleVoxelComponent>();
			const SubstanceID& subID = dvComp.SubstanceOverride.empty()
				? std::string(kDefaultSubstanceID) : dvComp.SubstanceOverride;
			const auto& sub = SubstanceRegistry::Get(subID);

			CrossEntityInfo info;
			info.EntityUUID = uuid;
			info.Flammable = sub.Flammable;
			info.SootResistance = sub.SootResistance;
			info.SootColor = sub.SootColor;
			info.BurnDuration = sub.BurnDuration;

			// Compute world AABB from destruction state grid or asset grid
			const voxelizer::VoxelGrid* grid = nullptr;
			auto stateIt = entityStates.find(uuid);
			if (stateIt != entityStates.end() && stateIt->second.GridInitialized)
				grid = &stateIt->second.ModifiedGrid;
			else if (entity.HasComponent<VoxelRendererComponent>()) {
				auto& vr = entity.GetComponent<VoxelRendererComponent>();
				if (vr.VoxelMeshUUID.IsValid()) {
					auto vmesh = AssetManager::GetAsset<VoxelMeshAsset>(vr.VoxelMeshUUID);
					if (vmesh)
						grid = &vmesh->GridData;
				}
			}

			if (grid && entity.HasComponent<TransformComponent>()) {
				auto worldMat = entity.GetComponent<TransformComponent>().GetTransform();
				glm::vec3 gridMin = grid->VoxelCenterToWorld(glm::ivec3(0));
				glm::vec3 gridMax = grid->VoxelCenterToWorld(grid->size - glm::ivec3(1));
				glm::vec3 wMin = glm::vec3(worldMat * glm::vec4(glm::min(gridMin, gridMax), 1.0f));
				glm::vec3 wMax = glm::vec3(worldMat * glm::vec4(glm::max(gridMin, gridMax), 1.0f));
				info.AABBMin = glm::min(wMin, wMax);
				info.AABBMax = glm::max(wMin, wMax);
			}
			else {
				info.AABBMin = glm::vec3(-1e6f);
				info.AABBMax = glm::vec3(1e6f);
			}

			ctx.Entities.push_back(info);
		}

		return ctx;
	}

	// =========================================================================
	// ProcessSpreadTicks (worker thread — time-budgeted)
	// =========================================================================
	std::vector<SpreadResult> VoxelSpreadSystem::ProcessSpreadTicks(
		std::vector<SpreadSnapshot> snapshots, CrossEntityContext crossCtx,
		std::vector<uint64_t>& outDeferred)
	{
		std::vector<SpreadResult> results;
		results.reserve(snapshots.size());

		auto start = std::chrono::steady_clock::now();
		for (size_t i = 0; i < snapshots.size(); i++) {
			results.push_back(ProcessSingleEntity(snapshots[i], crossCtx));

			auto elapsed = std::chrono::steady_clock::now() - start;
			if (std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() >= 4) {
				// Record remaining entities for priority next frame
				for (size_t j = i + 1; j < snapshots.size(); j++)
					outDeferred.push_back(snapshots[j].EntityUUID);
				break;
			}
		}

		return results;
	}

	// =========================================================================
	// ProcessSingleEntity (worker thread — iterates flat work items)
	// =========================================================================
	SpreadResult VoxelSpreadSystem::ProcessSingleEntity(const SpreadSnapshot& snap,
		const CrossEntityContext& crossCtx)
	{
		SpreadResult result;
		result.EntityUUID = snap.EntityUUID;

		static const glm::ivec3 neighbors[6] = {
			{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}
		};

		int crossEntityCount = 0;
		static constexpr int kMaxCrossEntityPerTick = 6;

		// Build a quick set of all burning coords in this entity for neighbor checks
		// (includes all active burns, not just this stride's work items)
		// We approximate with just the work items — burns not in work items are still burning
		// but we don't have the full map. Use a local set of work item coords.
		std::unordered_set<glm::ivec3, VoxelCoordHash> workCoords;
		workCoords.reserve(snap.WorkItems.size());
		for (const auto& item : snap.WorkItems)
			workCoords.insert(item.Coord);

		for (const auto& item : snap.WorkItems) {
			const glm::ivec3& coord = item.Coord;
			const ActiveBurn& burn = item.Burn;

			// Tick damage (burn stage tint is computed on main thread in AdvanceTimers)
			if (item.NeedsTick) {
				VoxelDamageEvent tickEvt;
				tickEvt.EntityUUID = snap.EntityUUID;
				tickEvt.GridCoord = coord;
				tickEvt.Type = burn.Type;
				tickEvt.RawAmount = snap.Substance.Health * 0.05f;
				result.TickDamage.push_back(tickEvt);
			}

			// Soot on non-burning neighbors
			if (item.NeedsSoot) {
				for (const auto& offset : neighbors) {
					glm::ivec3 nb = coord + offset;
					if (!snap.Grid.IsValidCoord(nb) || !snap.Grid.IsFilled(nb))
						continue;
					if (workCoords.count(nb))
						continue; // already burning (at least in this stride), skip soot

					float sootGain = 0.02f * (1.0f - snap.Substance.SootResistance);
					if (sootGain > 0.0f) {
						auto& tint = result.TintUpdates[nb];
						if (tint.Intensity < 0.6f) {
							tint.Color = snap.Substance.SootColor;
							tint.Intensity = glm::clamp(tint.Intensity + sootGain, 0.0f, 0.6f);
						}
					}
				}
			}

			// Spread to neighbors
			if (item.NeedsSpread && snap.Substance.PropagatesDamage) {
				bool isSurfaceVoxel = false;
				for (const auto& offset : neighbors) {
					glm::ivec3 nb = coord + offset;
					if (!snap.Grid.IsValidCoord(nb) || !snap.Grid.IsFilled(nb)) {
						isSurfaceVoxel = true;
						continue;
					}
					if (workCoords.count(nb)) continue;

					// Deterministic pseudo-random spread
					size_t h = VoxelCoordHash()(nb) ^ static_cast<size_t>(burn.Timer * 1000.0f);
					float roll = static_cast<float>(h % 1000) / 1000.0f;
					if (roll < snap.Substance.DamageSpreadFactor)
						result.NewIgnitions.push_back(nb);
				}

				// Cross-entity spread
				if (isSurfaceVoxel && crossEntityCount < kMaxCrossEntityPerTick) {
					glm::vec3 localPos = snap.Grid.VoxelCenterToWorld(coord);
					glm::vec3 worldPos = glm::vec3(snap.WorldTransform * glm::vec4(localPos, 1.0f));
					float spreadRadius = snap.Substance.CrossEntitySpreadRadius;

					for (const auto& other : crossCtx.Entities) {
						if (other.EntityUUID == snap.EntityUUID)
							continue;

						if (worldPos.x < other.AABBMin.x - spreadRadius ||
							worldPos.x > other.AABBMax.x + spreadRadius ||
							worldPos.y < other.AABBMin.y - spreadRadius ||
							worldPos.y > other.AABBMax.y + spreadRadius ||
							worldPos.z < other.AABBMin.z - spreadRadius ||
							worldPos.z > other.AABBMax.z + spreadRadius)
							continue;

						CrossEntityIgnition ignition;
						ignition.TargetEntityUUID = other.EntityUUID;
						ignition.WorldPos = worldPos;
						result.CrossIgnitions.push_back(ignition);
						crossEntityCount++;

						if (crossEntityCount >= kMaxCrossEntityPerTick)
							break;
					}
				}
			}
		}

		return result;
	}

	// =========================================================================
	// ApplyPendingResults — simplified, no burn state merge needed
	// =========================================================================
	void VoxelSpreadSystem::ApplyPendingResults(Scene* scene)
	{
		CB_PROFILE_SCOPE("VoxelSpreadSystem::ApplyPendingResults");
		auto& entityStates = VoxelDestructionSystem::GetEntityStates();

		for (auto& result : s_PendingResults) {
			auto stateIt = entityStates.find(result.EntityUUID);
			if (stateIt == entityStates.end()) continue;
			auto& state = stateIt->second;
			if (!state.GridInitialized) continue;

			// Apply tick damage events
			for (auto& dmgEvt : result.TickDamage) {
				if (!state.ModifiedGrid.IsValidCoord(dmgEvt.GridCoord) ||
					!state.ModifiedGrid.IsFilled(dmgEvt.GridCoord))
					continue;
				VoxelDestructionSystem::QueueDamage(dmgEvt);
			}

			// Apply new ignitions
			for (const auto& coord : result.NewIgnitions) {
				if (!state.ModifiedGrid.IsValidCoord(coord) ||
					!state.ModifiedGrid.IsFilled(coord))
					continue;
				if (state.ActiveBurns.count(coord)) continue;

				// Skip voxels that already finished burning
				auto dmgIt = state.DamageMap.find(coord);
				if (dmgIt != state.DamageMap.end() && dmgIt->second.Charred)
					continue;

				Entity entity = scene->GetEntityByUUID(UUID(result.EntityUUID));
				if (!entity || !entity.HasComponent<DestructibleVoxelComponent>()) continue;

				auto& dvComp = entity.GetComponent<DestructibleVoxelComponent>();
				const auto& sub = SubstanceRegistry::Get(
					dvComp.SubstanceOverride.empty() ? kDefaultSubstanceID : dvComp.SubstanceOverride);

				if (!sub.Flammable || sub.BurnDuration <= 0.0f) continue;

				ActiveBurn& burn = state.ActiveBurns[coord];
				burn.Type = VoxelDamageType::Fire;
				burn.Timer = sub.BurnDuration;
				burn.SpreadTimer = 0.25f;
				burn.TickTimer = 0.1f;
				burn.SootTimer = 0.15f;

				auto& health = state.DamageMap[coord];
				if (health.MaxHealth == 0.0f) {
					health.MaxHealth = sub.Health;
					health.CurrentHealth = sub.Health;
				}
				health.Burning = true;
				health.FireTimer = sub.BurnDuration;
			}

			// Merge tint updates
			bool tintChanged = false;
			for (auto& [coord, tintEntry] : result.TintUpdates) {
				if (!state.ModifiedGrid.IsValidCoord(coord) ||
					!state.ModifiedGrid.IsFilled(coord))
					continue;
				auto& existing = state.TintMap[coord];
				existing.Color = tintEntry.Color;
				existing.Intensity = glm::clamp(
					glm::max(existing.Intensity, tintEntry.Intensity), 0.0f, 1.0f);
				tintChanged = true;
			}

			if (tintChanged) {
				state.MeshDirty = true;
				VoxelDestructionSystem::MarkTintDirty(result.EntityUUID);
			}

			// Process cross-entity ignitions
			for (const auto& ignition : result.CrossIgnitions) {
				Entity targetEntity = scene->GetEntityByUUID(UUID(ignition.TargetEntityUUID));
				if (!targetEntity || !targetEntity.HasComponent<DestructibleVoxelComponent>()) continue;
				if (!targetEntity.HasComponent<TransformComponent>()) continue;

				auto& targetDV = targetEntity.GetComponent<DestructibleVoxelComponent>();
				const auto& targetSub = SubstanceRegistry::Get(
					targetDV.SubstanceOverride.empty() ? kDefaultSubstanceID : targetDV.SubstanceOverride);

				auto targetIt = entityStates.find(ignition.TargetEntityUUID);
				if (targetIt == entityStates.end() || !targetIt->second.GridInitialized) {
					// Target has no destruction state yet — queue a fire damage event
					// which will trigger EnsureGridInitialized + ignition in ProcessDamageQueue
					if (targetSub.Flammable && targetSub.BurnDuration > 0.0f) {
						// Need to convert world pos to grid coord using the asset grid
						auto& targetTC = targetEntity.GetComponent<TransformComponent>();
						glm::vec3 localPos = glm::vec3(
							glm::inverse(targetTC.GetTransform()) * glm::vec4(ignition.WorldPos, 1.0f));

						VoxelDamageEvent fireEvt;
						fireEvt.EntityUUID = ignition.TargetEntityUUID;
						fireEvt.GridCoord = glm::ivec3(glm::round(localPos)); // approximate
						fireEvt.Type = VoxelDamageType::Fire;
						fireEvt.RawAmount = 0.01f; // minimal damage, just to ignite
						VoxelDestructionSystem::QueueDamage(fireEvt);
					}
					continue;
				}

				auto& targetState = targetIt->second;

				// Convert world pos to target grid coord
				auto& targetTC = targetEntity.GetComponent<TransformComponent>();
				glm::vec3 localPos = glm::vec3(
					glm::inverse(targetTC.GetTransform()) * glm::vec4(ignition.WorldPos, 1.0f));
				glm::ivec3 gridCoord = targetState.ModifiedGrid.WorldToVoxel(localPos);

				if (!targetState.ModifiedGrid.IsValidCoord(gridCoord) ||
					!targetState.ModifiedGrid.IsFilled(gridCoord))
					continue;

				if (targetSub.Flammable && targetSub.BurnDuration > 0.0f) {
					if (!targetState.ActiveBurns.count(gridCoord)) {
						// Skip voxels that already finished burning
						auto tdmgIt = targetState.DamageMap.find(gridCoord);
						if (tdmgIt != targetState.DamageMap.end() && tdmgIt->second.Charred)
							continue;

						ActiveBurn& burn = targetState.ActiveBurns[gridCoord];
						burn.Type = VoxelDamageType::Fire;
						burn.Timer = targetSub.BurnDuration;
						burn.SpreadTimer = 0.25f;
						burn.TickTimer = 0.1f;
						burn.SootTimer = 0.15f;

						auto& health = targetState.DamageMap[gridCoord];
						if (health.MaxHealth == 0.0f) {
							health.MaxHealth = targetSub.Health;
							health.CurrentHealth = targetSub.Health;
						}
						health.Burning = true;
						health.FireTimer = targetSub.BurnDuration;
					}
				}
				else {
					float sootGain = 0.02f * (1.0f - targetSub.SootResistance);
					if (sootGain > 0.0f) {
						auto& tint = targetState.TintMap[gridCoord];
						tint.Color = targetSub.SootColor;
						tint.Intensity = glm::clamp(tint.Intensity + sootGain, 0.0f, 0.6f);
						targetState.MeshDirty = true;
						VoxelDestructionSystem::MarkTintDirty(ignition.TargetEntityUUID);
					}
				}
			}
		}

		s_PendingResults.clear();
		s_HasPendingResults = false;
	}

	// =========================================================================
	// Shutdown
	// =========================================================================
	void VoxelSpreadSystem::Shutdown()
	{
		if (s_WorkerRunning && s_WorkerFuture.valid()) {
			s_WorkerFuture.wait();
			s_WorkerRunning = false;
		}
		s_PendingResults.clear();
		s_HasPendingResults = false;
		s_StrideIndex = 0;
		s_DeferredEntities.clear();
		s_GridCache.clear();
	}
}
