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
#include "CBEngine/Voxel/Substance/SubstanceRegistry.h"
#include "CBEngine/Voxel/Substance/VoxelTintTypes.h"
#include "CBEngine/Voxel/Substance/HeatOctree.h"

#include "CBEngine/Debug/Instrumentor.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <chrono>

namespace CB
{
	// Helper: resolve substance from DVC override, then VRC vtex dominant substance, then default
	static SubstanceID ResolveEntitySubstanceID(const DestructibleVoxelComponent& dv, Entity entity)
	{
		if (!dv.SubstanceOverride.empty())
			return dv.SubstanceOverride;
		if (entity && entity.HasComponent<VoxelRendererComponent>()) {
			const auto& vr = entity.GetComponent<VoxelRendererComponent>();
			if (!vr.DominantSubstanceID.empty())
				return vr.DominantSubstanceID;
		}
		return SubstanceID(kDefaultSubstanceID);
	}

	// Static storage
	std::future<std::vector<SpreadResult>> VoxelSpreadSystem::s_WorkerFuture;
	bool VoxelSpreadSystem::s_WorkerRunning = false;
	std::vector<SpreadResult> VoxelSpreadSystem::s_PendingResults;
	bool VoxelSpreadSystem::s_HasPendingResults = false;

	size_t VoxelSpreadSystem::s_StrideIndex = 0;
	std::vector<uint64_t> VoxelSpreadSystem::s_DeferredEntities;
	std::unordered_map<uint64_t, VoxelSpreadSystem::CachedGrid> VoxelSpreadSystem::s_GridCache;

	// Octree heat system statics
	bool VoxelSpreadSystem::s_UseOctreeHeat = true;
	HeatOctree VoxelSpreadSystem::s_HeatOctree;
	int VoxelSpreadSystem::s_IgnitionCooldown = 0;
	float VoxelSpreadSystem::s_MinIgnitionTemp = 0.0f;

	// =========================================================================
	// IgniteVoxel — shared helper for igniting a single voxel
	// =========================================================================
	static void IgniteVoxel(EntityDestructionState& state, const glm::ivec3& coord,
	                        const VoxelSubstanceProperties& sub)
	{
		if (state.ActiveBurns.count(coord)) return;
		auto dmgIt = state.DamageMap.find(coord);
		if (dmgIt != state.DamageMap.end() && dmgIt->second.Charred) return;
		if (!sub.Flammable || sub.BurnDuration <= 0.0f) return;

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

		auto& tint = state.TintMap[coord];
		tint.Color = sub.BurnStageTints[1];
		tint.Intensity = 0.5f;
		state.MeshDirty = true;
	}

	// =========================================================================
	// ConductEntityHeat — per-voxel intra-entity heat conduction
	// Throttled: runs every kTickInterval frames with accumulated dt
	// =========================================================================
	void VoxelSpreadSystem::ConductEntityHeat(Scene* scene, float dt)
	{
		CB_PROFILE_SCOPE_CAT("VoxelSpreadSystem::ConductEntityHeat", "Voxel");

		// Throttle: accumulate dt, only process every N frames
		static constexpr int kTickInterval = 5;
		static float s_AccumDt = 0.0f;
		static int s_TickCounter = 0;
		s_AccumDt += dt;
		if (++s_TickCounter < kTickInterval) return;
		float accumDt = s_AccumDt;
		s_AccumDt = 0.0f;
		s_TickCounter = 0;

		auto& entityStates = VoxelDestructionSystem::GetEntityStates();

		static constexpr int kMaxOpsPerFrame = 40000;
		static constexpr float kMinHeatThreshold = 0.5f;
		static constexpr int kMaxHeatMapSize = 15000;
		static constexpr int kEvictBatchSize = 5000;
		static constexpr int kEvictCheckInterval = 20; // in ticks (= 60 frames)
		static int s_EvictCounter = 0;
		++s_EvictCounter;

		int totalOps = 0;

		static const glm::ivec3 neighbors[6] = {
			{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}
		};

		for (auto& [uuid, state] : entityStates) {
			if (state.ActiveBurns.empty() && state.PerVoxelHeat.empty()) continue;
			if (!state.GridInitialized) continue;

			Entity entity = scene->GetEntityByUUID(UUID(uuid));
			if (!entity || !entity.HasComponent<DestructibleVoxelComponent>()) continue;

			auto& dvComp = entity.GetComponent<DestructibleVoxelComponent>();
			const SubstanceID subID = ResolveEntitySubstanceID(dvComp, entity);
			const auto& sub = SubstanceRegistry::Get(subID);

			if (sub.InternalConductionRate <= 0.0f) continue;

			// Build burn coord set ONCE — reused by steps 3, 5, 6
			// Cheaper than repeated .count() calls on the unordered_map
			std::unordered_set<glm::ivec3, VoxelCoordHash> burnSet;
			burnSet.reserve(state.ActiveBurns.size());
			for (const auto& [coord, burn] : state.ActiveBurns)
				burnSet.insert(coord);

			// 1. Inject: burning voxels maintain temperature in heat map
			for (auto& [coord, burn] : state.ActiveBurns) {
				float burnTemp = sub.IgnitionTemperature + sub.BurnHeatOutput * burn.HeatRampFactor;
				auto& h = state.PerVoxelHeat[coord];
				h = glm::max(h, burnTemp);
			}

			// 2. Conduct heat along frontier
			std::vector<glm::ivec3> nextFrontier;
			std::vector<std::pair<glm::ivec3, float>> deltas;

			for (const auto& coord : state.HeatFrontier) {
				if (totalOps >= kMaxOpsPerFrame) break;

				auto heatIt = state.PerVoxelHeat.find(coord);
				if (heatIt == state.PerVoxelHeat.end() || heatIt->second < 1.0f) continue;
				float heat = heatIt->second;

				for (const auto& offset : neighbors) {
					glm::ivec3 nb = coord + offset;
					if (!state.ModifiedGrid.IsValidCoord(nb) || !state.ModifiedGrid.IsFilled(nb))
						continue;

					auto dmgIt = state.DamageMap.find(nb);
					if (dmgIt != state.DamageMap.end() && dmgIt->second.Charred)
						continue;

					auto nbIt = state.PerVoxelHeat.find(nb);
					float nbHeat = (nbIt != state.PerVoxelHeat.end()) ? nbIt->second : 0.0f;
					float diff = heat - nbHeat;
					if (diff > 1.0f) {
						float transfer = diff * sub.InternalConductionRate * accumDt;
						deltas.push_back({nb, transfer});
						nextFrontier.push_back(nb);
					}
					totalOps++;
				}
			}

			// 3. Merge deltas + check ignitions
			std::vector<glm::ivec3> pendingIgnitions;
			for (const auto& [coord, delta] : deltas) {
				float& h = state.PerVoxelHeat[coord];
				h += delta;
				if (h >= sub.IgnitionTemperature && !burnSet.count(coord)) {
					auto dmgIt = state.DamageMap.find(coord);
					if (!(dmgIt != state.DamageMap.end() && dmgIt->second.Charred))
						pendingIgnitions.push_back(coord);
				}
			}

			// 4. Apply ignitions (after loop — safe)
			for (const auto& coord : pendingIgnitions) {
				IgniteVoxel(state, coord, sub);
				burnSet.insert(coord);
				nextFrontier.push_back(coord);
			}

			// 5. Decay non-burning heat entries
			std::vector<glm::ivec3> toErase;
			for (auto& [coord, heat] : state.PerVoxelHeat) {
				if (!burnSet.count(coord)) {
					heat -= sub.HeatDecayRate * accumDt;
					if (heat < kMinHeatThreshold)
						toErase.push_back(coord);
				}
			}
			for (const auto& c : toErase)
				state.PerVoxelHeat.erase(c);

			// 6. Rebuild frontier for next frame
			// Scan burning voxels for cold neighbors — cap scan to avoid O(N*6) blowup
			static constexpr int kMaxFrontierScan = 4000;
			int scanned = 0;
			for (const auto& [coord, burn] : state.ActiveBurns) {
				if (++scanned > kMaxFrontierScan) break;
				for (const auto& offset : neighbors) {
					glm::ivec3 nb = coord + offset;
					if (state.ModifiedGrid.IsValidCoord(nb) && state.ModifiedGrid.IsFilled(nb)
						&& !burnSet.count(nb)) {
						auto dmgIt = state.DamageMap.find(nb);
						if (!(dmgIt != state.DamageMap.end() && dmgIt->second.Charred)) {
							nextFrontier.push_back(coord);
							break;
						}
					}
				}
			}
			// Also keep heated non-burning voxels still warm enough to conduct
			for (const auto& [coord, heat] : state.PerVoxelHeat) {
				if (heat > 1.0f && !burnSet.count(coord))
					nextFrontier.push_back(coord);
			}

			// Deduplicate frontier
			std::sort(nextFrontier.begin(), nextFrontier.end(),
				[](const glm::ivec3& a, const glm::ivec3& b) {
					if (a.x != b.x) return a.x < b.x;
					if (a.y != b.y) return a.y < b.y;
					return a.z < b.z;
				});
			nextFrontier.erase(std::unique(nextFrontier.begin(), nextFrontier.end()), nextFrontier.end());
			state.HeatFrontier = std::move(nextFrontier);

			// 7. Heat map size cap — evict coldest entries periodically
			if ((s_EvictCounter % kEvictCheckInterval) == 0
				&& static_cast<int>(state.PerVoxelHeat.size()) > kMaxHeatMapSize) {
				std::vector<std::pair<glm::ivec3, float>> sorted;
				sorted.reserve(state.PerVoxelHeat.size());
				for (const auto& [c, h] : state.PerVoxelHeat) {
					if (!burnSet.count(c))
						sorted.push_back({c, h});
				}
				std::sort(sorted.begin(), sorted.end(),
					[](const auto& a, const auto& b) { return a.second < b.second; });
				int evictCount = glm::min(kEvictBatchSize, static_cast<int>(sorted.size()));
				for (int i = 0; i < evictCount; ++i)
					state.PerVoxelHeat.erase(sorted[i].first);
			}

			if (totalOps >= kMaxOpsPerFrame) break;
		}
	}

	// =========================================================================
	// OnUpdate
	// =========================================================================
	void VoxelSpreadSystem::OnUpdate(Scene* scene, Timestep ts)
	{
		CB_PROFILE_SCOPE_CAT("VoxelSpreadSystem::OnUpdate", "Voxel");
		CollectWorkerResults();

		if (s_HasPendingResults)
			ApplyPendingResults(scene);

		if (!s_WorkerRunning) {
			float dt = glm::min(static_cast<float>(ts), 1.0f / 30.0f);
			AdvanceTimers(scene, dt);
			ConductEntityHeat(scene, dt);

			if (s_UseOctreeHeat) {
				// Octree-based heat system: inject, diffuse, decay, then check ignitions
				UpdateHeatOctree(scene, dt);
				ProcessIgnitionsFromHeat(scene);
			}

			auto snapshots = BuildSnapshots(scene);
			if (!snapshots.empty()) {
				if (s_UseOctreeHeat) {
					// In octree mode, skip neighbor spread — only tick damage + soot
					auto crossCtx = CrossEntityContext{}; // empty, not needed
					s_WorkerRunning = true;
					s_WorkerFuture = std::async(std::launch::async,
						[snaps = std::move(snapshots), ctx = std::move(crossCtx)]() mutable {
							std::vector<uint64_t> deferred;
							auto results = ProcessSpreadTicks(std::move(snaps), std::move(ctx), deferred);
							s_DeferredEntities = std::move(deferred);
							return results;
						});
				}
				else {
					// Legacy neighbor-based spread
					auto crossCtx = BuildCrossEntityContext(scene);
					s_WorkerRunning = true;
					s_WorkerFuture = std::async(std::launch::async,
						[snaps = std::move(snapshots), ctx = std::move(crossCtx)]() mutable {
							std::vector<uint64_t> deferred;
							auto results = ProcessSpreadTicks(std::move(snaps), std::move(ctx), deferred);
							s_DeferredEntities = std::move(deferred);
							return results;
						});
				}
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
		CB_PROFILE_SCOPE_CAT("VoxelSpreadSystem::AdvanceTimers", "Voxel");
		auto& entityStates = VoxelDestructionSystem::GetEntityStates();

		for (auto& [uuid, state] : entityStates) {
			if (state.ActiveBurns.empty()) continue;

			// Look up substance for burn stage tint interpolation
			Entity entity = scene->GetEntityByUUID(UUID(uuid));
			VoxelSubstanceProperties sub{};
			if (entity && entity.HasComponent<DestructibleVoxelComponent>()) {
				auto& dvComp = entity.GetComponent<DestructibleVoxelComponent>();
				const SubstanceID subID = ResolveEntitySubstanceID(dvComp, entity);
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
		CB_PROFILE_SCOPE_CAT("VoxelSpreadSystem::BuildSnapshots", "Voxel");
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
			const SubstanceID subID = ResolveEntitySubstanceID(dvComp, entity);

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
				bool needsSpread = s_UseOctreeHeat ? false : (burn.SpreadTimer <= 0.0f);
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
			const SubstanceID subID = ResolveEntitySubstanceID(dvComp, entity);
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
		CB_PROFILE_SCOPE_CAT("VoxelSpreadSystem::ApplyPendingResults", "Voxel");
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

			// Apply new ignitions (skipped in octree mode — ignitions come from ProcessIgnitionsFromHeat)
			if (!s_UseOctreeHeat)
			for (const auto& coord : result.NewIgnitions) {
				if (!state.ModifiedGrid.IsValidCoord(coord) ||
					!state.ModifiedGrid.IsFilled(coord))
					continue;

				Entity entity = scene->GetEntityByUUID(UUID(result.EntityUUID));
				if (!entity || !entity.HasComponent<DestructibleVoxelComponent>()) continue;

				auto& dvComp = entity.GetComponent<DestructibleVoxelComponent>();
				const auto& sub = SubstanceRegistry::Get(ResolveEntitySubstanceID(dvComp, entity));

				IgniteVoxel(state, coord, sub);
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

			// Process cross-entity ignitions (skipped in octree mode — handled by heat diffusion)
			if (!s_UseOctreeHeat)
			for (const auto& ignition : result.CrossIgnitions) {
				Entity targetEntity = scene->GetEntityByUUID(UUID(ignition.TargetEntityUUID));
				if (!targetEntity || !targetEntity.HasComponent<DestructibleVoxelComponent>()) continue;
				if (!targetEntity.HasComponent<TransformComponent>()) continue;

				auto& targetDV = targetEntity.GetComponent<DestructibleVoxelComponent>();
				const auto& targetSub = SubstanceRegistry::Get(ResolveEntitySubstanceID(targetDV, targetEntity));

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
	// UpdateHeatOctree — collect burning voxel heat sources, run octree cycle
	// =========================================================================
	void VoxelSpreadSystem::UpdateHeatOctree(Scene* scene, float dt)
	{
		CB_PROFILE_SCOPE_CAT("VoxelSpreadSystem::UpdateHeatOctree", "Voxel");

		// Initialize octree on first call
		if (!s_HeatOctree.IsInitialized()) {
			HeatOctree::Config config;
			config.MinCellSize = 0.2f;       // ~2x voxel size
			config.SubdivideHeatThreshold = 10.0f;
			config.CollapseThreshold = 5.0f;
			config.BaseDiffusionRate = 0.3f;
			config.BaseDecayRate = 5.0f;
			s_HeatOctree.Initialize(config);

			// Cache minimum ignition temperature across all flammable substances
			s_MinIgnitionTemp = 1e30f;
			for (const auto& id : SubstanceRegistry::GetAllIDs()) {
				const auto& sub = SubstanceRegistry::Get(id);
				if (sub.Flammable && sub.IgnitionTemperature > 0.0f)
					s_MinIgnitionTemp = glm::min(s_MinIgnitionTemp, sub.IgnitionTemperature);
			}
			if (s_MinIgnitionTemp >= 1e29f)
				s_MinIgnitionTemp = 100.0f; // fallback
		}

		// Collect heat sources from all active burns
		std::vector<HeatSourceInfo> sources;
		auto& entityStates = VoxelDestructionSystem::GetEntityStates();

		for (auto& [uuid, state] : entityStates) {
			if (state.ActiveBurns.empty() || !state.GridInitialized) continue;

			Entity entity = scene->GetEntityByUUID(UUID(uuid));
			if (!entity || !entity.HasComponent<DestructibleVoxelComponent>()) continue;
			if (!entity.HasComponent<TransformComponent>()) continue;

			auto& dvComp = entity.GetComponent<DestructibleVoxelComponent>();
			const SubstanceID subID = ResolveEntitySubstanceID(dvComp, entity);
			const auto& sub = SubstanceRegistry::Get(subID);

			glm::mat4 worldMat = entity.GetComponent<TransformComponent>().GetTransform();

			for (auto& [coord, burn] : state.ActiveBurns) {
				// Ramp up heat output over time
				burn.HeatRampFactor = glm::clamp(
					burn.HeatRampFactor + sub.HeatAcceleration * dt, 0.0f, 1.0f);

				float effectiveRate = sub.BurnHeatOutput * burn.HeatRampFactor;
				if (effectiveRate <= 0.0f) continue;

				glm::vec3 localPos = state.ModifiedGrid.VoxelCenterToWorld(coord);
				glm::vec3 worldPos = glm::vec3(worldMat * glm::vec4(localPos, 1.0f));

				// A burning voxel is at least at ignition temperature + combustion output.
				// MaxTemperature must exceed IgnitionTemperature for cross-entity fire to work.
				float maxTemp = sub.IgnitionTemperature + sub.BurnHeatOutput;
				sources.push_back({ worldPos, effectiveRate, maxTemp, sub.HeatConductivity });
			}
		}

		if (sources.empty() && s_HeatOctree.GetNodeCount() <= 1) return;

		s_HeatOctree.InjectHeat(sources, dt);
		s_HeatOctree.DiffuseHeat(dt);
		s_HeatOctree.DecayHeat(dt);
		s_HeatOctree.Compact();
		s_HeatOctree.RebuildNeighborCache();
	}

	// =========================================================================
	// ProcessIgnitionsFromHeat — throttled ignition from octree hot cells
	// =========================================================================
	void VoxelSpreadSystem::ProcessIgnitionsFromHeat(Scene* scene)
	{
		CB_PROFILE_SCOPE_CAT("VoxelSpreadSystem::ProcessIgnitionsFromHeat", "Voxel");

		// Run every 5 frames
		if (++s_IgnitionCooldown < 5) return;
		s_IgnitionCooldown = 0;

		if (!s_HeatOctree.IsInitialized()) return;

		// Query hot cells above minimum ignition temperature
		std::vector<HeatOctree::HotCell> hotCells;
		s_HeatOctree.QueryHotCells(s_MinIgnitionTemp, hotCells);
		if (hotCells.empty()) return;

		auto& entityStates = VoxelDestructionSystem::GetEntityStates();
		int totalIgnitions = 0;
		static constexpr int kMaxIgnitionsPerCall = 32;

		// --- Pre-compute per-entity data ONCE (avoids redundant work per hot cell) ---
		struct CachedEntityData
		{
			uint64_t UUID;
			float IgnitionTemp;
			const voxelizer::VoxelGrid* Grid;
			glm::mat4 InvWorld;
			// World-space AABB for broad-phase culling
			glm::vec3 AABBMin;
			glm::vec3 AABBMax;
		};

		std::vector<CachedEntityData> cachedEntities;
		auto view = scene->GetEntitiesWith<DestructibleVoxelComponent>();
		cachedEntities.reserve(16);

		for (auto enttID : view) {
			Entity entity(enttID, scene);
			if (!entity.HasComponent<TransformComponent>()) continue;

			auto& dvComp = entity.GetComponent<DestructibleVoxelComponent>();
			const SubstanceID subID = ResolveEntitySubstanceID(dvComp, entity);
			const auto& sub = SubstanceRegistry::Get(subID);

			if (!sub.Flammable || sub.BurnDuration <= 0.0f) continue;

			uint64_t uuid = static_cast<uint64_t>(entity.GetUUID());
			const voxelizer::VoxelGrid* grid = nullptr;
			auto stateIt = entityStates.find(uuid);

			if (stateIt != entityStates.end() && stateIt->second.GridInitialized) {
				grid = &stateIt->second.ModifiedGrid;
			}
			else if (entity.HasComponent<VoxelRendererComponent>()) {
				auto& vr = entity.GetComponent<VoxelRendererComponent>();
				if (vr.VoxelMeshUUID.IsValid()) {
					auto vmesh = AssetManager::GetAsset<VoxelMeshAsset>(vr.VoxelMeshUUID);
					if (vmesh) grid = &vmesh->GridData;
				}
			}
			if (!grid) continue;

			glm::mat4 worldMat = entity.GetComponent<TransformComponent>().GetTransform();

			// Compute world AABB for broad-phase culling
			glm::vec3 gridMin = grid->VoxelCenterToWorld(glm::ivec3(0));
			glm::vec3 gridMax = grid->VoxelCenterToWorld(grid->size - glm::ivec3(1));
			glm::vec3 localMin = glm::min(gridMin, gridMax);
			glm::vec3 localMax = glm::max(gridMin, gridMax);

			// Transform all 8 corners to find world AABB
			glm::vec3 wMin(1e30f), wMax(-1e30f);
			for (int c = 0; c < 8; c++) {
				glm::vec3 corner(
					(c & 1) ? localMax.x : localMin.x,
					(c & 2) ? localMax.y : localMin.y,
					(c & 4) ? localMax.z : localMin.z);
				glm::vec3 wc = glm::vec3(worldMat * glm::vec4(corner, 1.0f));
				wMin = glm::min(wMin, wc);
				wMax = glm::max(wMax, wc);
			}

			CachedEntityData data;
			data.UUID = uuid;
			data.IgnitionTemp = sub.IgnitionTemperature;
			data.Grid = grid;
			data.InvWorld = glm::inverse(worldMat);
			data.AABBMin = wMin;
			data.AABBMax = wMax;
			cachedEntities.push_back(data);
		}

		if (cachedEntities.empty()) return;

		// --- Iterate hot cells against pre-computed entity list ---
		for (const auto& cell : hotCells) {
			if (totalIgnitions >= kMaxIgnitionsPerCall) break;

			glm::vec3 cellMin = cell.Center - glm::vec3(cell.HalfSize);
			glm::vec3 cellMax = cell.Center + glm::vec3(cell.HalfSize);

			for (const auto& ed : cachedEntities) {
				if (totalIgnitions >= kMaxIgnitionsPerCall) break;

				// Broad-phase: AABB overlap test
				if (cellMax.x < ed.AABBMin.x || cellMin.x > ed.AABBMax.x ||
					cellMax.y < ed.AABBMin.y || cellMin.y > ed.AABBMax.y ||
					cellMax.z < ed.AABBMin.z || cellMin.z > ed.AABBMax.z)
					continue;

				// Heat threshold per substance
				if (cell.Heat < ed.IgnitionTemp) continue;

				// Convert cell center to local grid space (using cached inverse)
				glm::vec3 localCenter = glm::vec3(ed.InvWorld * glm::vec4(cell.Center, 1.0f));
				glm::ivec3 gridCenter = ed.Grid->WorldToVoxel(localCenter);

				auto stateIt = entityStates.find(ed.UUID);

				// Check a small neighborhood around the cell center
				for (int dx = -1; dx <= 1 && totalIgnitions < kMaxIgnitionsPerCall; dx++) {
					for (int dy = -1; dy <= 1 && totalIgnitions < kMaxIgnitionsPerCall; dy++) {
						for (int dz = -1; dz <= 1 && totalIgnitions < kMaxIgnitionsPerCall; dz++) {
							glm::ivec3 coord = gridCenter + glm::ivec3(dx, dy, dz);
							if (!ed.Grid->IsValidCoord(coord) || !ed.Grid->IsFilled(coord))
								continue;

							// Skip if already burning or charred
							if (stateIt != entityStates.end()) {
								auto& st = stateIt->second;
								if (st.ActiveBurns.count(coord)) continue;
								auto dmgIt = st.DamageMap.find(coord);
								if (dmgIt != st.DamageMap.end() && dmgIt->second.Charred) continue;
							}

							VoxelDamageEvent fireEvt;
							fireEvt.EntityUUID = ed.UUID;
							fireEvt.GridCoord = coord;
							fireEvt.Type = VoxelDamageType::Fire;
							fireEvt.RawAmount = 0.01f;
							VoxelDestructionSystem::QueueDamage(fireEvt);
							totalIgnitions++;
						}
					}
				}
			}
		}
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
		s_HeatOctree.Shutdown();
		s_IgnitionCooldown = 0;
		s_MinIgnitionTemp = 0.0f;
	}
}
