#include "cbpch.h"
#include "VoxelSpreadSystem.h"
#include "VoxelDestructionSystem.h"

#include "CBEngine/Scene/Scene.h"
#include "CBEngine/Scene/Entity.h"
#include "CBEngine/Components/TransformComponent.h"
#include "CBEngine/Components/DestructibleVoxelComponent.h"
#include "CBEngine/Voxel/Destruction/SubstanceRegistry.h"
#include "CBEngine/Voxel/Destruction/VoxelTintTypes.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <unordered_set>

namespace CB
{
	// Static storage
	std::future<std::vector<SpreadResult>> VoxelSpreadSystem::s_WorkerFuture;
	bool VoxelSpreadSystem::s_WorkerRunning = false;
	std::vector<SpreadResult> VoxelSpreadSystem::s_PendingResults;
	bool VoxelSpreadSystem::s_HasPendingResults = false;

	// =========================================================================
	// OnUpdate
	// =========================================================================
	void VoxelSpreadSystem::OnUpdate(Scene* scene,Timestep ts)
	{
		CollectWorkerResults();

		if (s_HasPendingResults)
			ApplyPendingResults(scene);

		if (!s_WorkerRunning) {
			float dt = glm::min(static_cast<float>(ts), 1.0f / 30.0f);
			auto snapshots = BuildSnapshots(scene, dt);
			if (!snapshots.empty()) {
				auto crossCtx = BuildCrossEntityContext(scene);
				s_WorkerRunning = true;
				s_WorkerFuture = std::async(std::launch::async, ProcessSpreadTicks,
					std::move(snapshots), std::move(crossCtx));
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
	// ApplyPendingResults
	// =========================================================================
	void VoxelSpreadSystem::ApplyPendingResults(Scene* scene)
	{
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

				Entity entity = scene->GetEntityByUUID(UUID(result.EntityUUID));
				if (!entity || !entity.HasComponent<DestructibleVoxelComponent>()) continue;

				auto& dvComp = entity.GetComponent<DestructibleVoxelComponent>();
				const auto& sub = SubstanceRegistry::Get(
					dvComp.SubstanceOverride.empty() ? kDefaultSubstanceID : dvComp.SubstanceOverride);

				if (!sub.Flammable || sub.BurnDuration <= 0.0f) continue;

				ActiveBurn& burn = state.ActiveBurns[coord];
				burn.Type = VoxelDamageType::Fire;
				burn.Timer = sub.BurnDuration;
				burn.SpreadTimer = 0.25f; // wait before spreading (natural wave)
				burn.TickTimer = 0.1f;

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

			// Merge burn states: update existing burns from worker results,
			// but preserve any NEW burns added by VoxelDestructionSystem since the snapshot
			{
				auto& updated = result.UpdatedBurns;

				// Build set of extinguished coords for quick lookup
				std::unordered_set<glm::ivec3, VoxelCoordHash> extinguishedSet(
					result.Extinguished.begin(), result.Extinguished.end());

				// Preserve burns added after the snapshot was taken
				for (auto& [coord, burn] : state.ActiveBurns) {
					if (updated.find(coord) == updated.end() &&
						extinguishedSet.find(coord) == extinguishedSet.end()) {
						// Not in worker results AND not extinguished — added after snapshot
						updated[coord] = burn;
					}
				}

				state.ActiveBurns = std::move(updated);
			}

			// Handle extinguished fires
			for (const auto& coord : result.Extinguished) {
				auto dmgIt = state.DamageMap.find(coord);
				if (dmgIt != state.DamageMap.end()) {
					dmgIt->second.Burning = false;
					dmgIt->second.FireTimer = 0.0f;
				}
			}

			if (tintChanged)
				state.MeshDirty = true;

			// Process cross-entity ignitions
			for (const auto& ignition : result.CrossIgnitions) {
				auto targetIt = entityStates.find(ignition.TargetEntityUUID);
				if (targetIt == entityStates.end()) continue;
				auto& targetState = targetIt->second;
				if (!targetState.GridInitialized) continue;

				Entity targetEntity = scene->GetEntityByUUID(UUID(ignition.TargetEntityUUID));
				if (!targetEntity || !targetEntity.HasComponent<DestructibleVoxelComponent>()) continue;
				if (!targetEntity.HasComponent<TransformComponent>()) continue;

				auto& targetDV = targetEntity.GetComponent<DestructibleVoxelComponent>();
				const auto& targetSub = SubstanceRegistry::Get(
					targetDV.SubstanceOverride.empty() ? kDefaultSubstanceID : targetDV.SubstanceOverride);

				// Convert world pos to target grid coord
				auto& targetTC = targetEntity.GetComponent<TransformComponent>();
				glm::vec3 localPos = glm::vec3(
					glm::inverse(targetTC.GetTransform()) * glm::vec4(ignition.WorldPos, 1.0f));
				glm::ivec3 gridCoord = targetState.ModifiedGrid.WorldToVoxel(localPos);

				if (!targetState.ModifiedGrid.IsValidCoord(gridCoord) ||
					!targetState.ModifiedGrid.IsFilled(gridCoord))
					continue;

				if (targetSub.Flammable && targetSub.BurnDuration > 0.0f) {
					// Ignite
					if (!targetState.ActiveBurns.count(gridCoord)) {
						ActiveBurn& burn = targetState.ActiveBurns[gridCoord];
						burn.Type = VoxelDamageType::Fire;
						burn.Timer = targetSub.BurnDuration;
						burn.SpreadTimer = 0.25f;
						burn.TickTimer = 0.1f;

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
					// Apply soot
					float sootGain = 0.02f * (1.0f - targetSub.SootResistance);
					if (sootGain > 0.0f) {
						auto& tint = targetState.TintMap[gridCoord];
						tint.Color = targetSub.SootColor;
						tint.Intensity = glm::clamp(tint.Intensity + sootGain, 0.0f, 0.6f);
						targetState.MeshDirty = true;
					}
				}
			}
		}

		s_PendingResults.clear();
		s_HasPendingResults = false;
	}

	// =========================================================================
	// BuildSnapshots
	// =========================================================================
	std::vector<SpreadSnapshot> VoxelSpreadSystem::BuildSnapshots(Scene* scene,float dt)
	{
		std::vector<SpreadSnapshot> snapshots;
		auto& entityStates = VoxelDestructionSystem::GetEntityStates();

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
			snap.Grid = state.ModifiedGrid;
			snap.Burns = state.ActiveBurns;
			snap.Substance = SubstanceRegistry::Get(subID);
			snap.DeltaTime = dt;

			if (entity.HasComponent<TransformComponent>())
				snap.WorldTransform = entity.GetComponent<TransformComponent>().GetTransform();

			snapshots.push_back(std::move(snap));
		}

		return snapshots;
	}

	// =========================================================================
	// BuildCrossEntityContext
	// =========================================================================
	CrossEntityContext VoxelSpreadSystem::BuildCrossEntityContext(Scene* scene)
	{
		CrossEntityContext ctx;
		auto& entityStates = VoxelDestructionSystem::GetEntityStates();

		for (auto& [uuid, state] : entityStates) {
			if (!state.GridInitialized) continue;

			Entity entity = scene->GetEntityByUUID(UUID(uuid));
			if (!entity || !entity.HasComponent<DestructibleVoxelComponent>()) continue;

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

			// Compute world AABB
			if (entity.HasComponent<TransformComponent>()) {
				auto worldMat = entity.GetComponent<TransformComponent>().GetTransform();
				glm::vec3 gridMin = state.ModifiedGrid.VoxelCenterToWorld(glm::ivec3(0));
				glm::vec3 gridMax = state.ModifiedGrid.VoxelCenterToWorld(state.ModifiedGrid.size - glm::ivec3(1));
				// Transform corners to world space (simplified: use 2 corners for AABB)
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
	// ProcessSpreadTicks (worker thread — pure function)
	// =========================================================================
	std::vector<SpreadResult> VoxelSpreadSystem::ProcessSpreadTicks(
		std::vector<SpreadSnapshot> snapshots, CrossEntityContext crossCtx)
	{
		std::vector<SpreadResult> results;
		results.reserve(snapshots.size());
		for (const auto& snap : snapshots)
			results.push_back(ProcessSingleEntity(snap, crossCtx));
		return results;
	}

	// =========================================================================
	// ProcessSingleEntity (worker thread — pure function)
	// =========================================================================
	SpreadResult VoxelSpreadSystem::ProcessSingleEntity(const SpreadSnapshot& snap,
		const CrossEntityContext& crossCtx)
	{
		SpreadResult result;
		result.EntityUUID = snap.EntityUUID;
		result.UpdatedBurns = snap.Burns;

		static const glm::ivec3 neighbors[6] = {
			{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}
		};

		int crossEntityCount = 0;
		static constexpr int kMaxCrossEntityPerTick = 6;

		for (auto it = result.UpdatedBurns.begin(); it != result.UpdatedBurns.end();) {
			const glm::ivec3& coord = it->first;
			ActiveBurn& burn = it->second;

			// Count down burn timer
			burn.Timer -= snap.DeltaTime;
			if (burn.Timer <= 0.0f) {
				result.Extinguished.push_back(coord);
				it = result.UpdatedBurns.erase(it);
				continue;
			}

			// Tick damage (every 0.2s)
			burn.TickTimer -= snap.DeltaTime;
			if (burn.TickTimer <= 0.0f) {
				burn.TickTimer = 0.2f;

				VoxelDamageEvent tickEvt;
				tickEvt.EntityUUID = snap.EntityUUID;
				tickEvt.GridCoord = coord;
				tickEvt.Type = burn.Type;
				tickEvt.RawAmount = snap.Substance.Health * 0.05f;
				result.TickDamage.push_back(tickEvt);

				// --- Burn stage tint interpolation ---
				float burnProgress = 1.0f - (burn.Timer / glm::max(snap.Substance.BurnDuration, 0.001f));
				burnProgress = glm::clamp(burnProgress, 0.0f, 1.0f);

				int stageIdx = 0;
				for (int s = 2; s >= 0; --s) {
					if (burnProgress >= snap.Substance.BurnStageThresholds[s]) {
						stageIdx = s + 1;
						break;
					}
				}

				float stageStart = (stageIdx > 0) ? snap.Substance.BurnStageThresholds[stageIdx - 1] : 0.0f;
				float stageEnd = (stageIdx < 3) ? snap.Substance.BurnStageThresholds[stageIdx] : 1.0f;
				float t = glm::clamp(
					(burnProgress - stageStart) / glm::max(stageEnd - stageStart, 0.001f), 0.0f, 1.0f);
				int nextStage = glm::min(stageIdx + 1, 3);
				Vector3 tintColor = glm::mix(
					snap.Substance.BurnStageTints[stageIdx],
					snap.Substance.BurnStageTints[nextStage], t);

				auto& tint = result.TintUpdates[coord];
				tint.Color = tintColor;
				tint.Intensity = 1.0f; // full intensity when actively burning
			}

			// --- Soot on non-burning neighbors (runs every tick, outside PropagatesDamage guard) ---
			// Apply soot to filled neighbors that aren't already burning
			for (const auto& offset : neighbors) {
				glm::ivec3 nb = coord + offset;
				if (!snap.Grid.IsValidCoord(nb) || !snap.Grid.IsFilled(nb))
					continue;
				if (result.UpdatedBurns.count(nb))
					continue; // already burning, skip soot

				float sootGain = 0.02f * (1.0f - snap.Substance.SootResistance);
				if (sootGain > 0.0f) {
					auto& tint = result.TintUpdates[nb];
					// Only apply soot if not already tinted more strongly
					if (tint.Intensity < 0.6f) {
						tint.Color = snap.Substance.SootColor;
						tint.Intensity = glm::clamp(tint.Intensity + sootGain, 0.0f, 0.6f);
					}
				}
			}

			// --- Spread to neighbors (every 0.25s, if substance allows) ---
			if (snap.Substance.PropagatesDamage) {
				burn.SpreadTimer -= snap.DeltaTime;
				if (burn.SpreadTimer <= 0.0f) {
					burn.SpreadTimer = 0.25f;

					bool isSurfaceVoxel = false;
					for (const auto& offset : neighbors) {
						glm::ivec3 nb = coord + offset;
						if (!snap.Grid.IsValidCoord(nb) || !snap.Grid.IsFilled(nb)) {
							isSurfaceVoxel = true; // empty/OOB neighbor = surface face
							continue;
						}
						if (result.UpdatedBurns.count(nb)) continue;

						// Deterministic pseudo-random spread to filled neighbor
						size_t h = VoxelCoordHash()(nb) ^ static_cast<size_t>(burn.Timer * 1000.0f);
						float roll = static_cast<float>(h % 1000) / 1000.0f;
						if (roll < snap.Substance.DamageSpreadFactor)
							result.NewIgnitions.push_back(nb);
					}

					// --- Cross-entity spread ---
					if (isSurfaceVoxel && crossEntityCount < kMaxCrossEntityPerTick) {
						glm::vec3 localPos = snap.Grid.VoxelCenterToWorld(coord);
						glm::vec3 worldPos = glm::vec3(snap.WorldTransform * glm::vec4(localPos, 1.0f));
						float spreadRadius = snap.Substance.CrossEntitySpreadRadius;

						for (const auto& other : crossCtx.Entities) {
							if (other.EntityUUID == snap.EntityUUID)
								continue;

							// AABB proximity check (inflate by spread radius)
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

			++it;
		}

		return result;
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
	}
}
