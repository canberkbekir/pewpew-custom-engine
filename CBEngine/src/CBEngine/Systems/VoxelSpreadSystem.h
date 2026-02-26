#pragma once

#include "ISystem.h"
#include "CBEngine/Core/TimeStep.h"
#include "CBEngine/Voxel/Destruction/VoxelDamageMap.h"
#include "CBEngine/Voxel/Destruction/VoxelSubstance.h"

#include <future>
#include <chrono>
#include <vector>

namespace CB
{
    class Scene;

    // Snapshot of one entity's burn state for worker thread
    struct SpreadSnapshot
    {
        uint64_t EntityUUID;
        voxelizer::VoxelGrid Grid;          // deep copy (read-only on worker)
        VoxelBurnMap Burns;                  // deep copy
        VoxelSubstanceProperties Substance;
        float DeltaTime;
    };

    // Results from one entity's spread tick
    struct SpreadResult
    {
        uint64_t EntityUUID;
        std::vector<VoxelDamageEvent> TickDamage;   // burn tick damage to queue
        std::vector<glm::ivec3> NewIgnitions;       // neighbors to ignite
        VoxelTintMap TintUpdates;                    // tint changes to merge
        VoxelBurnMap UpdatedBurns;                   // new burn state
        std::vector<glm::ivec3> Extinguished;       // fires that expired
    };

    class VoxelSpreadSystem
    {
    public:
        static void OnUpdate(Scene* scene, Timestep ts);
        static void Shutdown();

    private:
        // Main thread: apply previous frame's results, snapshot current state
        static void CollectWorkerResults();
        static void ApplyPendingResults(Scene* scene);
        static std::vector<SpreadSnapshot> BuildSnapshots(Scene* scene, float dt);

        // Worker thread: process spread ticks (pure function, no shared state)
        static std::vector<SpreadResult> ProcessSpreadTicks(
            std::vector<SpreadSnapshot> snapshots);
        static SpreadResult ProcessSingleEntity(const SpreadSnapshot& snap);

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
        void OnUpdate(Scene* scene, Timestep ts) override { VoxelSpreadSystem::OnUpdate(scene, ts); }
        void Shutdown() override { VoxelSpreadSystem::Shutdown(); }
        const char* GetName() const override { return "VoxelSpreadSystem"; }
        int GetPriority() const override { return 176; }
    };
}
