#include "cbpch.h"
#include "VoxelSpreadSystem.h"
#include "VoxelDestructionSystem.h"

#include "CBEngine/Scene/Scene.h"
#include "CBEngine/Scene/Entity.h"
#include "CBEngine/Components/DestructibleVoxelComponent.h"
#include "CBEngine/Voxel/Destruction/VoxelSubstanceDatabase.h"
#include "CBEngine/Voxel/Destruction/VoxelTintTypes.h"

#include <glm/glm.hpp>
#include <algorithm>

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
    void VoxelSpreadSystem::OnUpdate(Scene* scene, Timestep ts)
    {
        // Step 1: Check if previous worker finished (non-blocking)
        CollectWorkerResults();

        // Step 2: Apply collected results to main-thread state
        if (s_HasPendingResults)
            ApplyPendingResults(scene);

        // Step 3: Only launch new worker if previous one is done
        if (!s_WorkerRunning)
        {
            float dt = glm::min(static_cast<float>(ts), 1.0f / 30.0f);
            auto snapshots = BuildSnapshots(scene, dt);
            if (!snapshots.empty())
            {
                s_WorkerRunning = true;
                s_WorkerFuture = std::async(std::launch::async, ProcessSpreadTicks, std::move(snapshots));
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

        for (auto& result : s_PendingResults)
        {
            auto stateIt = entityStates.find(result.EntityUUID);
            if (stateIt == entityStates.end()) continue;
            auto& state = stateIt->second;
            if (!state.GridInitialized) continue;

            // Apply tick damage events
            for (auto& dmgEvt : result.TickDamage)
            {
                // Validate coord is still valid/filled
                if (!state.ModifiedGrid.IsValidCoord(dmgEvt.GridCoord) ||
                    !state.ModifiedGrid.IsFilled(dmgEvt.GridCoord))
                    continue;
                VoxelDestructionSystem::QueueDamage(dmgEvt);
            }

            // Apply new ignitions
            for (const auto& coord : result.NewIgnitions)
            {
                if (!state.ModifiedGrid.IsValidCoord(coord) ||
                    !state.ModifiedGrid.IsFilled(coord))
                    continue;
                // Don't re-ignite already burning
                if (state.ActiveBurns.count(coord)) continue;

                // Resolve substance for burn duration
                Entity entity = scene->GetEntityByUUID(UUID(result.EntityUUID));
                if (!entity || !entity.HasComponent<DestructibleVoxelComponent>()) continue;

                auto& dvComp = entity.GetComponent<DestructibleVoxelComponent>();
                VoxelMaterialType matType = VoxelMaterialType::Stone;
                if (!dvComp.SubstanceOverride.empty())
                    matType = VoxelSubstanceDatabase::ResolveSubstanceName(dvComp.SubstanceOverride);
                const auto& sub = VoxelSubstanceDatabase::Get(matType);

                if (!sub.Flammable || sub.BurnDuration <= 0.0f) continue;

                ActiveBurn& burn = state.ActiveBurns[coord];
                burn.Type = VoxelDamageType::Fire;
                burn.Timer = sub.BurnDuration;
                burn.SpreadTimer = 0.5f;
                burn.TickTimer = 0.2f;

                // Set burning flag in damage map
                auto& health = state.DamageMap[coord];
                if (health.MaxHealth == 0.0f)
                {
                    health.MaxHealth = sub.Health;
                    health.CurrentHealth = sub.Health;
                }
                health.Burning = true;
                health.FireTimer = sub.BurnDuration;
            }

            // Merge tint updates
            bool tintChanged = false;
            for (auto& [coord, tintEntry] : result.TintUpdates)
            {
                if (!state.ModifiedGrid.IsValidCoord(coord) ||
                    !state.ModifiedGrid.IsFilled(coord))
                    continue;
                auto& existing = state.TintMap[coord];
                existing.Color = tintEntry.Color;
                existing.Intensity = glm::clamp(
                    glm::max(existing.Intensity, tintEntry.Intensity), 0.0f, 1.0f);
                tintChanged = true;
            }

            // Update burn states
            state.ActiveBurns = std::move(result.UpdatedBurns);

            // Handle extinguished fires
            for (const auto& coord : result.Extinguished)
            {
                auto dmgIt = state.DamageMap.find(coord);
                if (dmgIt != state.DamageMap.end())
                {
                    dmgIt->second.Burning = false;
                    dmgIt->second.FireTimer = 0.0f;
                }
            }

            // Mark mesh dirty if tint changed
            if (tintChanged)
                state.MeshDirty = true;
        }

        s_PendingResults.clear();
        s_HasPendingResults = false;
    }

    // =========================================================================
    // BuildSnapshots
    // =========================================================================
    std::vector<SpreadSnapshot> VoxelSpreadSystem::BuildSnapshots(Scene* scene, float dt)
    {
        std::vector<SpreadSnapshot> snapshots;
        auto& entityStates = VoxelDestructionSystem::GetEntityStates();

        for (auto& [uuid, state] : entityStates)
        {
            if (state.ActiveBurns.empty() || !state.GridInitialized)
                continue;

            Entity entity = scene->GetEntityByUUID(UUID(uuid));
            if (!entity || !entity.HasComponent<DestructibleVoxelComponent>())
                continue;

            auto& dvComp = entity.GetComponent<DestructibleVoxelComponent>();
            VoxelMaterialType matType = VoxelMaterialType::Stone;
            if (!dvComp.SubstanceOverride.empty())
                matType = VoxelSubstanceDatabase::ResolveSubstanceName(dvComp.SubstanceOverride);

            SpreadSnapshot snap;
            snap.EntityUUID = uuid;
            snap.Grid = state.ModifiedGrid; // deep copy
            snap.Burns = state.ActiveBurns; // deep copy
            snap.Substance = VoxelSubstanceDatabase::Get(matType);
            snap.DeltaTime = dt;
            snapshots.push_back(std::move(snap));
        }

        return snapshots;
    }

    // =========================================================================
    // ProcessSpreadTicks (worker thread — pure function)
    // =========================================================================
    std::vector<SpreadResult> VoxelSpreadSystem::ProcessSpreadTicks(
        std::vector<SpreadSnapshot> snapshots)
    {
        std::vector<SpreadResult> results;
        results.reserve(snapshots.size());
        for (const auto& snap : snapshots)
            results.push_back(ProcessSingleEntity(snap));
        return results;
    }

    // =========================================================================
    // ProcessSingleEntity (worker thread — pure function)
    // =========================================================================
    SpreadResult VoxelSpreadSystem::ProcessSingleEntity(const SpreadSnapshot& snap)
    {
        SpreadResult result;
        result.EntityUUID = snap.EntityUUID;
        result.UpdatedBurns = snap.Burns;

        for (auto it = result.UpdatedBurns.begin(); it != result.UpdatedBurns.end(); )
        {
            const glm::ivec3& coord = it->first;
            ActiveBurn& burn = it->second;

            // Count down burn timer
            burn.Timer -= snap.DeltaTime;
            if (burn.Timer <= 0.0f)
            {
                result.Extinguished.push_back(coord);
                it = result.UpdatedBurns.erase(it);
                continue;
            }

            // Tick damage (every 0.2s)
            burn.TickTimer -= snap.DeltaTime;
            if (burn.TickTimer <= 0.0f)
            {
                burn.TickTimer = 0.2f;

                VoxelDamageEvent tickEvt;
                tickEvt.EntityUUID = snap.EntityUUID;
                tickEvt.GridCoord = coord;
                tickEvt.Type = burn.Type;
                tickEvt.RawAmount = snap.Substance.Health * 0.05f;
                result.TickDamage.push_back(tickEvt);

                // Deepen tint on burning voxel
                auto tintCfgIt = snap.Substance.DamageTints.find(burn.Type);
                if (tintCfgIt != snap.Substance.DamageTints.end())
                {
                    auto& tint = result.TintUpdates[coord];
                    tint.Color = tintCfgIt->second.Color;
                    tint.Intensity = glm::clamp(tint.Intensity + 0.05f, 0.0f, 1.0f);
                }
            }

            // Spread to neighbors (every 0.5s, if substance allows)
            if (snap.Substance.PropagatesDamage)
            {
                burn.SpreadTimer -= snap.DeltaTime;
                if (burn.SpreadTimer <= 0.0f)
                {
                    burn.SpreadTimer = 0.5f;

                    static const glm::ivec3 neighbors[6] = {
                        {1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}
                    };
                    for (const auto& offset : neighbors)
                    {
                        glm::ivec3 nb = coord + offset;
                        if (!snap.Grid.IsValidCoord(nb) || !snap.Grid.IsFilled(nb))
                            continue;
                        if (result.UpdatedBurns.count(nb)) continue;

                        // Deterministic pseudo-random spread
                        size_t h = VoxelCoordHash()(nb) ^ static_cast<size_t>(burn.Timer * 1000.0f);
                        float roll = static_cast<float>(h % 1000) / 1000.0f;
                        if (roll < snap.Substance.DamageSpreadFactor)
                            result.NewIgnitions.push_back(nb);
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
        if (s_WorkerRunning && s_WorkerFuture.valid())
        {
            s_WorkerFuture.wait();
            s_WorkerRunning = false;
        }
        s_PendingResults.clear();
        s_HasPendingResults = false;
    }
}
