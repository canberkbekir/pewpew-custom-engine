#include "cbpch.h"
#include "VoxelDestructionSystem.h"

#include "CBEngine/Scene/Scene.h"
#include "CBEngine/Scene/Entity.h"
#include "CBEngine/Components/TransformComponent.h"
#include "CBEngine/Components/VoxelRendererComponent.h"
#include "CBEngine/Components/ColliderComponent.h"
#include "CBEngine/Components/RigidBodyComponent.h"
#include "CBEngine/Components/CoreComponents.h"
#include "CBEngine/Components/DestructibleVoxelComponent.h"
#include "CBEngine/Components/VoxelAnchorComponent.h"
#include "CBEngine/Asset/AssetManager.h"
#include "CBEngine/Asset/ProcessedMeshAsset.h"
#include "CBEngine/Physics/VoxelSplitter.h"
#include "CBEngine/Physics/VoxelCollisionShapeGenerator.h"
#include "CBEngine/Physics/PhysicsWorld.h"
#include "CBEngine/Utils/VoxelizerAPI.h"
#include "CBEngine/Voxel/Destruction/VoxelSubstanceDatabase.h"
#include "CBEngine/Voxel/Destruction/VoxelStructuralIntegrity.h"
#include "CBEngine/Scripting/ScriptEngine.h"

#include <glm/glm.hpp>
#include <algorithm>
#include <numeric>
#include <cmath>

namespace CB
{
    // =========================================================================
    // Static storage
    // =========================================================================
    std::vector<VoxelDamageEvent>                        VoxelDestructionSystem::s_DamageQueue;
    std::unordered_map<uint64_t, EntityDestructionState> VoxelDestructionSystem::s_EntityStates;
    std::vector<uint64_t>                                VoxelDestructionSystem::s_StructuralCheckQueue;

    // =========================================================================
    // LoadSubstances
    // =========================================================================
    void VoxelDestructionSystem::LoadSubstances(const std::string& path)
    {
        VoxelSubstanceDatabase::Load(path);
    }

    // =========================================================================
    // QueueDamage — thread-safe append
    // Note: called from Lua (main thread) and potentially Jolt callbacks.
    // For full thread safety a mutex should be added if Jolt callbacks run
    // on worker threads. For now, all Jolt contact processing happens on the
    // main thread in this engine so the plain push_back is safe.
    // =========================================================================
    void VoxelDestructionSystem::QueueDamage(const VoxelDamageEvent& event)
    {
        s_DamageQueue.push_back(event);
    }

    // =========================================================================
    // GetEntityGrid — debug visualization accessor
    // =========================================================================
    const voxelizer::VoxelGrid* VoxelDestructionSystem::GetEntityGrid(uint64_t entityUUID)
    {
        auto it = s_EntityStates.find(entityUUID);
        if (it != s_EntityStates.end() && it->second.GridInitialized)
            return &it->second.ModifiedGrid;
        return nullptr;
    }

    // =========================================================================
    // Shutdown
    // =========================================================================
    void VoxelDestructionSystem::Shutdown()
    {
        s_DamageQueue.clear();
        s_EntityStates.clear();
        s_StructuralCheckQueue.clear();
    }

    // =========================================================================
    // OnEntityDestroyed — cleanup when DestructibleVoxelComponent is removed
    // =========================================================================
    void VoxelDestructionSystem::OnEntityDestroyed(uint64_t uuid)
    {
        s_EntityStates.erase(uuid);
        s_StructuralCheckQueue.erase(
            std::remove(s_StructuralCheckQueue.begin(), s_StructuralCheckQueue.end(), uuid),
            s_StructuralCheckQueue.end());
    }

    // =========================================================================
    // OnUpdate — main per-frame driver
    // =========================================================================
    void VoxelDestructionSystem::OnUpdate(Scene* scene, Timestep ts)
    {
        ProcessDamageQueue(scene);
        ProcessPendingRemovals(scene);
        ProcessStructuralIntegrity(scene);
    }

    // =========================================================================
    // ResolveEffectiveDamage — apply per-type substance resistances
    // =========================================================================
    float VoxelDestructionSystem::ResolveEffectiveDamage(float raw, VoxelDamageType type,
                                                          const VoxelSubstanceProperties& sub,
                                                          float resistanceMultiplier)
    {
        float multiplier = 1.0f;

        switch (type)
        {
        case VoxelDamageType::Impact:
            // Hardness 0-100: at 100 hardness only 1/(1+1)=0.5 of damage gets through,
            // at 0 hardness the full amount gets through
            multiplier = 1.0f / (1.0f + sub.Hardness * 0.01f);
            break;
        case VoxelDamageType::Explosion:
            multiplier = 1.0f - sub.ExplosionResistance;
            break;
        case VoxelDamageType::Slice:
            multiplier = 1.0f - sub.SliceResistance;
            break;
        case VoxelDamageType::Fire:
        case VoxelDamageType::Acid:
        case VoxelDamageType::Pressure:
        case VoxelDamageType::Structural:
        default:
            multiplier = 1.0f;
            break;
        }

        // ResistanceMultiplier > 1.0 = tougher object, so divide damage
        float effective = raw * multiplier;
        if (resistanceMultiplier > 0.0f)
            effective /= resistanceMultiplier;

        return glm::max(0.0f, effective);
    }

    // =========================================================================
    // EnsureGridInitialized — copy the vmesh asset grid into the entity state
    // Returns false if the entity or asset can't be found.
    // =========================================================================
    bool VoxelDestructionSystem::EnsureGridInitialized(Scene* scene, uint64_t entityUUID,
                                                        EntityDestructionState& state)
    {
        if (state.GridInitialized)
            return true;

        Entity entity = scene->GetEntityByUUID(UUID(entityUUID));
        if (!entity)
            return false;

        if (!entity.HasComponent<VoxelRendererComponent>())
            return false;

        auto& vr = entity.GetComponent<VoxelRendererComponent>();
        if (!vr.VoxelMeshUUID.IsValid())
            return false;

        auto vmesh = AssetManager::GetAsset<VoxelMeshAsset>(vr.VoxelMeshUUID);
        if (!vmesh)
            return false;

        // Deep copy — we own this copy and mutate it as voxels die
        state.ModifiedGrid            = vmesh->GridData;
        state.ModifiedPaletteIndices  = vmesh->PaletteIndices;
        state.GridInitialized         = true;
        return true;
    }

    // =========================================================================
    // ProcessDamageQueue — apply damage, update health, collect dead voxels
    // =========================================================================
    void VoxelDestructionSystem::ProcessDamageQueue(Scene* scene)
    {
        if (s_DamageQueue.empty())
            return;

        auto queue = std::move(s_DamageQueue);
        s_DamageQueue.clear();

        for (auto event : queue) // copy — we may modify GridCoord below
        {
            Entity entity = scene->GetEntityByUUID(UUID(event.EntityUUID));
            if (!entity)
                continue;

            if (!entity.HasComponent<DestructibleVoxelComponent>())
                continue;

            auto& dvComp = entity.GetComponent<DestructibleVoxelComponent>();
            if (!dvComp.Enabled)
                continue;

            // Resolve substance for this entity/voxel
            VoxelMaterialType matType = VoxelMaterialType::Stone;
            if (!dvComp.SubstanceOverride.empty())
                matType = VoxelSubstanceDatabase::ResolveSubstanceName(dvComp.SubstanceOverride);
            // TODO Phase 4: per-voxel lookup via VoxelTextureAsset

            const VoxelSubstanceProperties& sub = VoxelSubstanceDatabase::Get(matType);

            // Impact threshold check
            if (event.Type == VoxelDamageType::Impact && event.RawAmount < sub.ImpactThreshold)
                continue;

            float effective = ResolveEffectiveDamage(
                event.RawAmount, event.Type, sub, dvComp.ResistanceMultiplier);

            if (effective <= 0.0f)
                continue;

            // Ensure grid is ready before we track damage
            auto& state = s_EntityStates[event.EntityUUID];
            if (!EnsureGridInitialized(scene, event.EntityUUID, state))
                continue;

            // Convert world-space hit position to grid coordinates using the grid's
            // actual transform (origin + voxelSize), not naive rounding.
            if (event.UseWorldPos)
            {
                event.GridCoord = state.ModifiedGrid.WorldToVoxel(event.WorldHitPos);
                if (!state.ModifiedGrid.IsValidCoord(event.GridCoord) ||
                    !state.ModifiedGrid.IsFilled(event.GridCoord))
                    continue;
            }

            // Fetch or create health entry for this coord
            auto& health = state.DamageMap[event.GridCoord];
            if (health.MaxHealth == 0.0f)
            {
                // First time this voxel is damaged — initialise
                health.MaxHealth     = sub.Health * dvComp.HealthMultiplier;
                health.CurrentHealth = health.MaxHealth;
                health.FractureStage = 0;
            }

            health.CurrentHealth = glm::max(0.0f, health.CurrentHealth - effective);

            // Update fracture stage
            float ratio = health.CurrentHealth / health.MaxHealth;
            if (ratio <= 0.0f)
                health.FractureStage = 4;
            else if (ratio <= sub.FractureThreshold * 0.2f)
                health.FractureStage = 3;
            else if (ratio <= sub.FractureThreshold * 0.5f)
                health.FractureStage = 2;
            else if (ratio <= sub.FractureThreshold)
                health.FractureStage = 1;
            else
                health.FractureStage = 0;

            // Fire Lua events
            float normHealth = (health.MaxHealth > 0.0f) ? (health.CurrentHealth / health.MaxHealth) : 0.0f;

            // Queue for removal if fully dead
            if (health.CurrentHealth <= 0.0f)
            {
                state.PendingRemoval.push_back(event.GridCoord);
                state.DamageMap.erase(event.GridCoord);

                ScriptEngine::OnVoxelDestroyed(event.EntityUUID,
                    event.GridCoord.x, event.GridCoord.y, event.GridCoord.z);
            }
            else
            {
                ScriptEngine::OnVoxelDamaged(event.EntityUUID,
                    event.GridCoord.x, event.GridCoord.y, event.GridCoord.z,
                    normHealth, effective);
            }
        }
    }

    // =========================================================================
    // ProcessPendingRemovals — remove dead voxels and rebuild/split entities
    // =========================================================================
    void VoxelDestructionSystem::ProcessPendingRemovals(Scene* scene)
    {
        // Collect entity UUIDs that have pending removals this frame
        std::vector<uint64_t> toDo;
        for (auto& [uuid, state] : s_EntityStates)
        {
            if (!state.PendingRemoval.empty())
                toDo.push_back(uuid);
        }

        for (uint64_t uuid : toDo)
        {
            auto it = s_EntityStates.find(uuid);
            if (it == s_EntityStates.end())
                continue;

            EntityDestructionState& state = it->second;
            if (state.PendingRemoval.empty())
                continue;

            Entity entity = scene->GetEntityByUUID(UUID(uuid));
            if (!entity)
            {
                s_EntityStates.erase(it);
                continue;
            }

            if (!entity.HasComponent<DestructibleVoxelComponent>())
                continue;

            auto& dvComp = entity.GetComponent<DestructibleVoxelComponent>();

            // Respect per-frame budget
            int budget = dvComp.MaxDestructionsPerFrame;
            std::vector<glm::ivec3> batch;
            if (static_cast<int>(state.PendingRemoval.size()) <= budget)
            {
                batch = std::move(state.PendingRemoval);
                state.PendingRemoval.clear();
            }
            else
            {
                batch.insert(batch.end(),
                    state.PendingRemoval.begin(),
                    state.PendingRemoval.begin() + budget);
                state.PendingRemoval.erase(
                    state.PendingRemoval.begin(),
                    state.PendingRemoval.begin() + budget);
            }

            // Remove from working grid
            uint64_t remaining = VoxelSplitter::RemoveVoxels(
                state.ModifiedGrid, state.ModifiedPaletteIndices, batch);

            if (remaining == 0)
            {
                // Fully destroyed
                CB_CORE_INFO("VoxelDestructionSystem: Entity {0} fully destroyed", uuid);
                scene->DestroyEntity(entity);
                s_EntityStates.erase(it);
                continue;
            }

            // Rebuild mesh in-place (no fragmentation check yet — structural
            // integrity below handles splitting unsupported parts)
            RebuildOrFragment(scene, uuid, state);

            // Queue for structural integrity check
            if (dvComp.StructuralIntegrityEnabled)
                s_StructuralCheckQueue.push_back(uuid);
        }
    }

    // =========================================================================
    // RebuildOrFragment — after voxels are removed, check for splits
    // =========================================================================
    void VoxelDestructionSystem::RebuildOrFragment(Scene* scene, uint64_t entityUUID,
                                                    EntityDestructionState& state)
    {
        Entity entity = scene->GetEntityByUUID(UUID(entityUUID));
        if (!entity)
            return;

        if (!entity.HasComponent<VoxelRendererComponent>() ||
            !entity.HasComponent<TransformComponent>())
            return;

        auto& vr = entity.GetComponent<VoxelRendererComponent>();
        auto& transform = entity.GetComponent<TransformComponent>();

        auto fragments = VoxelSplitter::FindConnectedComponents(
            state.ModifiedGrid, state.ModifiedPaletteIndices);

        if (fragments.size() <= 1)
        {
            // Single connected piece — update the entity's mesh in-place
            Ref<Mesh> newMesh;
            if (!state.ModifiedPaletteIndices.empty())
                newMesh = VoxelizerAPI::CreatePaletteMeshFromGrid(
                    state.ModifiedGrid, state.ModifiedPaletteIndices);
            else
                newMesh = VoxelizerAPI::CreateMeshFromGrid(state.ModifiedGrid);

            if (newMesh)
            {
                vr.MeshAsset = newMesh;

                if (auto* physWorld = scene->GetPhysicsWorld())
                    physWorld->GetShapeCache().Invalidate(vr.VoxelMeshUUID);

                if (entity.HasComponent<ColliderComponent>())
                    entity.GetComponent<ColliderComponent>().ShapeDirty = true;
            }
            return;
        }

        // Multiple fragments — destroy original, spawn one entity per fragment
        CB_CORE_INFO("VoxelDestructionSystem: Entity {0} split into {1} fragments",
            entityUUID, fragments.size());

        // Save properties before destroying
        Vector3  origPos       = transform.GetWorldPosition();
        Vector3  origRot       = transform.Rotation;
        Vector3  origScale     = transform.Scale;
        String   origName      = entity.GetComponent<TagComponent>().Tag;
        bool     hadPhysics    = entity.HasComponent<RigidBodyComponent>();
        float    origFriction  = 0.5f;
        float    origRestitution = 0.3f;
        BodyType origBodyType  = BodyType::Dynamic;

        if (hadPhysics)
        {
            auto& rb    = entity.GetComponent<RigidBodyComponent>();
            origBodyType    = rb.Type;
            origFriction    = rb.Friction;
            origRestitution = rb.Restitution;
        }

        auto paletteColorTex    = vr.PaletteColorTexture;
        auto paletteMaterialTex = vr.PaletteMaterialTexture;
        bool hasPalette         = vr.HasPalette;

        // Fix 4: Save DestructibleVoxelComponent before destroying original
        DestructibleVoxelComponent origDV;
        if (entity.HasComponent<DestructibleVoxelComponent>())
            origDV = entity.GetComponent<DestructibleVoxelComponent>();

        scene->DestroyEntity(entity);
        s_EntityStates.erase(entityUUID);

        // Fix 5: Sort fragments by voxel count (biggest first) and enforce MaxFragmentsPerObject
        std::vector<size_t> sortedIndices(fragments.size());
        std::iota(sortedIndices.begin(), sortedIndices.end(), 0);
        std::sort(sortedIndices.begin(), sortedIndices.end(),
            [&](size_t a, size_t b) { return fragments[a].VoxelCount > fragments[b].VoxelCount; });

        int maxFragments = origDV.MaxFragmentsPerObject;
        int spawned = 0;

        for (size_t idx : sortedIndices)
        {
            const auto& frag = fragments[idx];
            if (frag.VoxelCount == 0)
                continue;

            if (spawned >= maxFragments)
                break; // discard smallest pieces beyond limit

            Entity fragEntity = scene->CreateEntity(origName + "_frag" + std::to_string(idx));

            auto& ft = fragEntity.GetComponent<TransformComponent>();
            // Use original entity's transform — mesh/collision vertices are in
            // grid-local space, same as the original entity expected.
            ft.Position = origPos;
            ft.Rotation = origRot;
            ft.Scale    = origScale;

            auto& fragVR = fragEntity.AddComponent<VoxelRendererComponent>();
            fragVR.PaletteColorTexture    = paletteColorTex;
            fragVR.PaletteMaterialTexture = paletteMaterialTex;
            fragVR.HasPalette             = hasPalette;
            fragVR.Visible                = true;

            if (!frag.PaletteIndices.empty())
                fragVR.MeshAsset = VoxelizerAPI::CreatePaletteMeshFromGrid(
                    frag.Grid, frag.PaletteIndices);
            else
                fragVR.MeshAsset = VoxelizerAPI::CreateMeshFromGrid(frag.Grid);

            auto& fragRB      = fragEntity.AddComponent<RigidBodyComponent>();
            fragRB.Type       = BodyType::Dynamic;
            {
                const auto& vs = frag.Grid.voxelSize;
                float voxelVolume = vs.x * vs.y * vs.z;
                constexpr float DENSITY = 100.0f; // kg/m^3
                fragRB.Mass = static_cast<float>(frag.VoxelCount) * voxelVolume * DENSITY;
            }
            fragRB.Friction   = origFriction;
            fragRB.Restitution = origRestitution;
            fragRB.UseGravity = true;

            auto& fragCol    = fragEntity.AddComponent<ColliderComponent>();
            fragCol.Shape    = ColliderShape::VoxelCompound;
            fragCol.ShapeDirty = true;

            auto compoundShape = VoxelCollisionShapeGenerator::GenerateFromGrid(frag.Grid);
            if (compoundShape)
            {
                fragCol.RuntimeShape = compoundShape;
                fragCol.ShapeDirty   = false;
                CB_CORE_INFO("Fragment {0}: collision shape created, {1} voxels, grid origin ({2},{3},{4}), scale ({5},{6},{7})",
                    idx, frag.VoxelCount,
                    frag.Grid.origin.x, frag.Grid.origin.y, frag.Grid.origin.z,
                    origScale.x, origScale.y, origScale.z);
            }
            else
            {
                CB_CORE_WARN("Fragment {0}: collision shape FAILED, {1} voxels", idx, frag.VoxelCount);
            }

            // Fix 4: Add DestructibleVoxelComponent (fragments are damageable)
            auto& fragDV = fragEntity.AddComponent<DestructibleVoxelComponent>();
            fragDV = origDV;
            fragDV.StructuralIntegrityEnabled = false; // prevent recursive cascade

            // Fix 4: Pre-initialize destruction state so EnsureGridInitialized works
            // (fragments have no VoxelMeshUUID — state must be set directly)
            auto& fragState = s_EntityStates[static_cast<uint64_t>(fragEntity.GetUUID())];
            fragState.ModifiedGrid           = frag.Grid;
            fragState.ModifiedPaletteIndices = frag.PaletteIndices;
            fragState.GridInitialized        = true;

            ++spawned;
        }
    }

    // =========================================================================
    // ProcessStructuralIntegrity — check for floating clusters after voxel removal
    // =========================================================================
    void VoxelDestructionSystem::ProcessStructuralIntegrity(Scene* scene)
    {
        if (s_StructuralCheckQueue.empty())
            return;

        auto checkQueue = std::move(s_StructuralCheckQueue);
        s_StructuralCheckQueue.clear();

        // Deduplicate
        std::sort(checkQueue.begin(), checkQueue.end());
        checkQueue.erase(std::unique(checkQueue.begin(), checkQueue.end()), checkQueue.end());

        for (uint64_t uuid : checkQueue)
        {
            auto stateIt = s_EntityStates.find(uuid);
            if (stateIt == s_EntityStates.end())
                continue;

            EntityDestructionState& state = stateIt->second;
            if (!state.GridInitialized)
                continue;

            Entity entity = scene->GetEntityByUUID(UUID(uuid));
            if (!entity)
                continue;

            if (!entity.HasComponent<DestructibleVoxelComponent>())
                continue;

            auto& dvComp = entity.GetComponent<DestructibleVoxelComponent>();
            if (!dvComp.StructuralIntegrityEnabled)
                continue;

            // Determine anchor voxels based on mode
            std::vector<glm::ivec3> anchors;
            if (dvComp.Anchor == DestructibleVoxelComponent::AnchorMode::BottomFace)
            {
                anchors = VoxelStructuralIntegrity::ComputeBottomFaceAnchors(state.ModifiedGrid);
            }
            else if (dvComp.Anchor == DestructibleVoxelComponent::AnchorMode::Manual)
            {
                // Find VoxelAnchorComponent entities nearby and convert to grid coords
                auto anchorView = scene->GetRegistry().view<VoxelAnchorComponent, TransformComponent>();
                for (auto anchorEntt : anchorView)
                {
                    Entity anchorEntity = { anchorEntt, scene };
                    auto& anchorComp = anchorEntity.GetComponent<VoxelAnchorComponent>();
                    auto& anchorTransform = anchorEntity.GetComponent<TransformComponent>();
                    glm::vec3 anchorWorldPos = anchorTransform.GetWorldPosition();

                    // Convert anchor world position to grid coord
                    glm::ivec3 gridCoord = state.ModifiedGrid.WorldToVoxel(anchorWorldPos);
                    if (state.ModifiedGrid.IsValidCoord(gridCoord))
                    {
                        // Add all filled voxels within a small radius of the anchor point
                        int r = glm::max(1, static_cast<int>(anchorComp.Radius));
                        for (int dx = -r; dx <= r; ++dx)
                        for (int dy = -r; dy <= r; ++dy)
                        for (int dz = -r; dz <= r; ++dz)
                        {
                            glm::ivec3 c = gridCoord + glm::ivec3(dx, dy, dz);
                            if (state.ModifiedGrid.IsValidCoord(c) && state.ModifiedGrid.IsFilled(c))
                                anchors.push_back(c);
                        }
                    }
                }
            }
            // AnchorMode::None → no structural check (skip)
            else
            {
                continue;
            }

            // Fix 6: When all anchors are destroyed, everything is unsupported.
            // FindUnsupportedClusters handles empty anchors by treating all
            // filled voxels as unsupported clusters — they collapse as physics debris.

            // BFS budget: limit iterations to avoid frame stalls
            constexpr int BFS_BUDGET = 50000;
            auto clusters = VoxelStructuralIntegrity::FindUnsupportedClusters(
                state.ModifiedGrid, anchors, BFS_BUDGET);

            if (clusters.empty())
                continue;

            CB_CORE_INFO("VoxelDestructionSystem: Entity {0} has {1} unsupported cluster(s)",
                uuid, clusters.size());

            // Fire Lua structural collapse event
            int totalCollapseVoxels = 0;
            for (const auto& cluster : clusters)
                totalCollapseVoxels += static_cast<int>(cluster.VoxelCount);
            ScriptEngine::OnStructuralCollapse(uuid,
                static_cast<int>(clusters.size()), totalCollapseVoxels);

            // Fix 5: Sort clusters by voxel count (biggest first) and enforce MaxFragmentsPerObject
            std::sort(clusters.begin(), clusters.end(),
                [](const CollapseCluster& a, const CollapseCluster& b)
                { return a.VoxelCount > b.VoxelCount; });

            int maxChunks = dvComp.MaxFragmentsPerObject;
            int chunkCount = 0;
            for (const auto& cluster : clusters)
            {
                if (chunkCount >= maxChunks)
                {
                    // Remove excess cluster voxels from grid (they vanish as "dust")
                    VoxelSplitter::RemoveVoxels(state.ModifiedGrid, state.ModifiedPaletteIndices,
                                                cluster.Coords);
                    for (const auto& c : cluster.Coords)
                        state.DamageMap.erase(c);
                    continue;
                }
                SpawnCollapsedChunk(scene, uuid, cluster, state);
                ++chunkCount;
            }

            // Rebuild the remaining mesh after removing collapsed voxels
            uint64_t remaining = state.ModifiedGrid.CountFilled();
            if (remaining == 0)
            {
                scene->DestroyEntity(entity);
                s_EntityStates.erase(stateIt);
            }
            else
            {
                RebuildOrFragment(scene, uuid, state);
            }
        }
    }

    // =========================================================================
    // SpawnCollapsedChunk — create a physics entity from a floating cluster
    // =========================================================================
    void VoxelDestructionSystem::SpawnCollapsedChunk(Scene* scene, uint64_t sourceEntityUUID,
                                                      const CollapseCluster& cluster,
                                                      EntityDestructionState& state)
    {
        Entity sourceEntity = scene->GetEntityByUUID(UUID(sourceEntityUUID));
        if (!sourceEntity)
            return;

        if (!sourceEntity.HasComponent<VoxelRendererComponent>() ||
            !sourceEntity.HasComponent<TransformComponent>())
            return;

        auto& vr = sourceEntity.GetComponent<VoxelRendererComponent>();

        // Extract palette indices for cluster voxels BEFORE RemoveVoxels erases them
        std::vector<uint8_t> chunkPaletteIndices;
        if (!state.ModifiedPaletteIndices.empty())
        {
            // Build filled-index map for source grid (same linear order as GetFilledCoords)
            std::unordered_map<uint64_t, size_t> sourceFilledMap;
            size_t filledIdx = 0;
            for (uint64_t i = 0; i < state.ModifiedGrid.totalVoxels; ++i)
            {
                if (state.ModifiedGrid.IsFilled(i))
                    sourceFilledMap[i] = filledIdx++;
            }

            chunkPaletteIndices.reserve(cluster.Coords.size());
            for (const auto& c : cluster.Coords)
            {
                if (!state.ModifiedGrid.IsValidCoord(c) || !state.ModifiedGrid.IsFilled(c))
                    continue;
                uint64_t idx = state.ModifiedGrid.CoordToIndex(c);
                auto it = sourceFilledMap.find(idx);
                if (it != sourceFilledMap.end() && it->second < state.ModifiedPaletteIndices.size())
                    chunkPaletteIndices.push_back(state.ModifiedPaletteIndices[it->second]);
                else
                    chunkPaletteIndices.push_back(0);
            }
        }

        // Remove cluster voxels from the working grid
        VoxelSplitter::RemoveVoxels(state.ModifiedGrid, state.ModifiedPaletteIndices,
                                    cluster.Coords);

        // Also remove from damage map
        for (const auto& c : cluster.Coords)
            state.DamageMap.erase(c);

        // Build a sub-grid for the collapsed chunk
        voxelizer::VoxelGrid tempGrid;
        tempGrid.size = state.ModifiedGrid.size;
        tempGrid.origin = state.ModifiedGrid.origin;
        tempGrid.voxelSize = state.ModifiedGrid.voxelSize;
        tempGrid.totalVoxels = state.ModifiedGrid.totalVoxels;
        uint64_t dataSize = (tempGrid.totalVoxels + 63) / 64;
        tempGrid.data.resize(dataSize, 0);

        for (const auto& c : cluster.Coords)
        {
            if (tempGrid.IsValidCoord(c))
                tempGrid.SetFilled(c);
        }

        // Build mesh from the temporary grid, using palette data if available
        Ref<Mesh> chunkMesh;
        if (!chunkPaletteIndices.empty())
            chunkMesh = VoxelizerAPI::CreatePaletteMeshFromGrid(tempGrid, chunkPaletteIndices);
        else
            chunkMesh = VoxelizerAPI::CreateMeshFromGrid(tempGrid);
        if (!chunkMesh)
            return;

        // Copy visual properties from source entity
        auto paletteColorTex    = vr.PaletteColorTexture;
        auto paletteMaterialTex = vr.PaletteMaterialTexture;
        bool hasPalette         = vr.HasPalette;

        auto& srcTransform = sourceEntity.GetComponent<TransformComponent>();
        Vector3 origRot    = srcTransform.Rotation;
        Vector3 origScale  = srcTransform.Scale;

        // Fix 7: Copy physics properties from source entity
        float srcFriction    = 0.5f;
        float srcRestitution = 0.3f;
        if (sourceEntity.HasComponent<RigidBodyComponent>())
        {
            auto& srcRB    = sourceEntity.GetComponent<RigidBodyComponent>();
            srcFriction    = srcRB.Friction;
            srcRestitution = srcRB.Restitution;
        }

        // Spawn the collapsed chunk entity
        Entity chunkEntity = scene->CreateEntity(
            sourceEntity.GetComponent<TagComponent>().Tag + "_collapse");

        auto& ct = chunkEntity.GetComponent<TransformComponent>();
        // Use original entity's transform — mesh/collision vertices are in
        // grid-local space, same as the original entity expected.
        ct.Position = srcTransform.Position;
        ct.Rotation = origRot;
        ct.Scale    = origScale;

        auto& chunkVR = chunkEntity.AddComponent<VoxelRendererComponent>();
        chunkVR.PaletteColorTexture    = paletteColorTex;
        chunkVR.PaletteMaterialTexture = paletteMaterialTex;
        chunkVR.HasPalette             = hasPalette;
        chunkVR.Visible                = true;
        chunkVR.MeshAsset              = chunkMesh;

        auto& rb      = chunkEntity.AddComponent<RigidBodyComponent>();
        rb.Type       = BodyType::Dynamic;
        {
            const auto& vs = state.ModifiedGrid.voxelSize;
            float voxelVolume = vs.x * vs.y * vs.z;
            constexpr float DENSITY = 100.0f; // kg/m^3
            rb.Mass = static_cast<float>(cluster.VoxelCount) * voxelVolume * DENSITY;
        }
        rb.UseGravity  = true;
        rb.Friction    = srcFriction;
        rb.Restitution = srcRestitution;

        auto& col    = chunkEntity.AddComponent<ColliderComponent>();
        col.Shape    = ColliderShape::VoxelCompound;
        col.ShapeDirty = true;

        auto compoundShape = VoxelCollisionShapeGenerator::GenerateFromGrid(tempGrid);
        if (compoundShape)
        {
            col.RuntimeShape = compoundShape;
            col.ShapeDirty   = false;
        }

        // Fix 4: Add DestructibleVoxelComponent (chunks are damageable)
        if (sourceEntity.HasComponent<DestructibleVoxelComponent>())
        {
            auto& chunkDV = chunkEntity.AddComponent<DestructibleVoxelComponent>();
            chunkDV = sourceEntity.GetComponent<DestructibleVoxelComponent>();
            chunkDV.StructuralIntegrityEnabled = false; // prevent recursive cascade
        }

        // Fix 4: Pre-initialize destruction state for chunk
        auto& chunkState = s_EntityStates[static_cast<uint64_t>(chunkEntity.GetUUID())];
        chunkState.ModifiedGrid           = tempGrid;
        chunkState.ModifiedPaletteIndices = chunkPaletteIndices;
        chunkState.GridInitialized        = true;

        CB_CORE_TRACE("  Spawned collapse chunk: {0} voxels, mass={1:.1f}",
            cluster.VoxelCount, rb.Mass);
    }

    // =========================================================================
    // QuerySphere — spatial query for the Lua VoxelQuery.Sphere API
    // Returns all filled voxels within radius of origin across ALL destructible
    // entities in the scene.
    // =========================================================================
    std::vector<VoxelHit> VoxelDestructionSystem::QuerySphere(Scene* scene,
                                                               glm::vec3 origin,
                                                               float radius)
    {
        std::vector<VoxelHit> hits;
        if (!scene)
            return hits;

        float radiusSq = radius * radius;

        auto view = scene->GetRegistry().view<VoxelRendererComponent, DestructibleVoxelComponent, TransformComponent>();
        for (auto enttEntity : view)
        {
            Entity entity = { enttEntity, scene };
            if (!entity) continue;

            auto& dvComp = entity.GetComponent<DestructibleVoxelComponent>();
            if (!dvComp.Enabled) continue;

            uint64_t uuid = entity.GetUUID();

            // Get grid — prefer modified state, fall back to asset
            const voxelizer::VoxelGrid* grid = nullptr;
            auto stateIt = s_EntityStates.find(uuid);
            if (stateIt != s_EntityStates.end() && stateIt->second.GridInitialized)
            {
                grid = &stateIt->second.ModifiedGrid;
            }
            else
            {
                auto& vr = entity.GetComponent<VoxelRendererComponent>();
                if (!vr.VoxelMeshUUID.IsValid()) continue;
                auto vmesh = AssetManager::GetAsset<VoxelMeshAsset>(vr.VoxelMeshUUID);
                if (!vmesh) continue;
                grid = &vmesh->GridData;
            }

            auto& transform = entity.GetComponent<TransformComponent>();
            glm::mat4 worldMatrix = transform.GetTransform();

            // Fix 14: Entity AABB early rejection
            {
                glm::vec3 gridLocalMin = grid->origin;
                glm::vec3 gridLocalMax = grid->origin + glm::vec3(grid->size) * grid->voxelSize;
                glm::vec3 localCenter  = (gridLocalMin + gridLocalMax) * 0.5f;
                glm::vec3 localHalf    = (gridLocalMax - gridLocalMin) * 0.5f;
                glm::vec3 worldCenter  = glm::vec3(worldMatrix * glm::vec4(localCenter, 1.0f));
                float maxHalf = glm::max(glm::max(localHalf.x, localHalf.y), localHalf.z);
                float entityDist = glm::distance(origin, worldCenter);
                if (entityDist > radius + maxHalf * 1.732f) // sqrt(3) conservative bound
                    continue;
            }

            // Fix 13: Range clamping — transform sphere into grid-local space
            glm::mat4 worldInv = glm::inverse(worldMatrix);
            glm::vec3 localOrigin = glm::vec3(worldInv * glm::vec4(origin, 1.0f));
            glm::ivec3 centerVoxel = grid->WorldToVoxel(localOrigin);
            float avgVoxelSize = (grid->voxelSize.x + grid->voxelSize.y + grid->voxelSize.z) / 3.0f;
            int r = static_cast<int>(std::ceil(radius / avgVoxelSize)) + 1;

            int w = grid->size.x;
            int h = grid->size.y;
            int d = grid->size.z;
            int xMin = glm::max(0, centerVoxel.x - r);
            int yMin = glm::max(0, centerVoxel.y - r);
            int zMin = glm::max(0, centerVoxel.z - r);
            int xMax = glm::min(w - 1, centerVoxel.x + r);
            int yMax = glm::min(h - 1, centerVoxel.y + r);
            int zMax = glm::min(d - 1, centerVoxel.z + r);

            // Resolve substance name for hits
            VoxelMaterialType matType = VoxelMaterialType::Stone;
            if (!dvComp.SubstanceOverride.empty())
                matType = VoxelSubstanceDatabase::ResolveSubstanceName(dvComp.SubstanceOverride);
            std::string substanceName = VoxelMaterialTypeToString(matType);

            for (int x = xMin; x <= xMax; ++x)
            for (int y = yMin; y <= yMax; ++y)
            for (int z = zMin; z <= zMax; ++z)
            {
                if (!grid->IsFilled(x, y, z))
                    continue;

                glm::vec3 localPos = grid->VoxelCenterToWorld(glm::ivec3(x, y, z));
                glm::vec3 worldPos = glm::vec3(worldMatrix * glm::vec4(localPos, 1.0f));
                glm::vec3 delta    = worldPos - origin;
                if (glm::dot(delta, delta) > radiusSq)
                    continue;

                glm::ivec3 coord(x, y, z);
                VoxelHit hit;
                hit.EntityUUID       = uuid;
                hit.WorldPos         = worldPos;
                hit.GridCoord        = coord;
                hit.SubstanceName    = substanceName;
                hit.NormalizedHealth = GetNormalizedHealth(uuid, coord);
                hits.push_back(hit);
            }
        }

        return hits;
    }

    // =========================================================================
    // QueryBox — spatial query for the Lua VoxelQuery.Box API
    // =========================================================================
    std::vector<VoxelHit> VoxelDestructionSystem::QueryBox(Scene* scene,
                                                            glm::vec3 center,
                                                            glm::vec3 halfExtents)
    {
        std::vector<VoxelHit> hits;
        if (!scene)
            return hits;

        glm::vec3 boxMin = center - halfExtents;
        glm::vec3 boxMax = center + halfExtents;

        auto view = scene->GetRegistry().view<VoxelRendererComponent, DestructibleVoxelComponent, TransformComponent>();
        for (auto enttEntity : view)
        {
            Entity entity = { enttEntity, scene };
            if (!entity) continue;

            auto& dvComp = entity.GetComponent<DestructibleVoxelComponent>();
            if (!dvComp.Enabled) continue;

            uint64_t uuid = entity.GetUUID();

            // Get grid — prefer modified state, fall back to asset
            const voxelizer::VoxelGrid* grid = nullptr;
            auto stateIt = s_EntityStates.find(uuid);
            if (stateIt != s_EntityStates.end() && stateIt->second.GridInitialized)
            {
                grid = &stateIt->second.ModifiedGrid;
            }
            else
            {
                auto& vr = entity.GetComponent<VoxelRendererComponent>();
                if (!vr.VoxelMeshUUID.IsValid()) continue;
                auto vmesh = AssetManager::GetAsset<VoxelMeshAsset>(vr.VoxelMeshUUID);
                if (!vmesh) continue;
                grid = &vmesh->GridData;
            }

            auto& transform = entity.GetComponent<TransformComponent>();
            glm::mat4 worldMatrix = transform.GetTransform();

            // Fix 14: Entity AABB early rejection
            {
                glm::vec3 gridLocalMin = grid->origin;
                glm::vec3 gridLocalMax = grid->origin + glm::vec3(grid->size) * grid->voxelSize;
                glm::vec3 localCenter  = (gridLocalMin + gridLocalMax) * 0.5f;
                glm::vec3 localHalf    = (gridLocalMax - gridLocalMin) * 0.5f;
                glm::vec3 worldCenter  = glm::vec3(worldMatrix * glm::vec4(localCenter, 1.0f));
                float maxHalf = glm::max(glm::max(localHalf.x, localHalf.y), localHalf.z);
                float maxQueryHalf = glm::max(glm::max(halfExtents.x, halfExtents.y), halfExtents.z);
                float entityDist = glm::distance(center, worldCenter);
                if (entityDist > maxQueryHalf + maxHalf * 1.732f)
                    continue;
            }

            // Transform query box into entity-local space for range clamping
            glm::mat4 worldInv = glm::inverse(worldMatrix);
            glm::vec3 localCenter = glm::vec3(worldInv * glm::vec4(center, 1.0f));
            // Conservative: use max half extent to handle rotation
            float maxHalfExt = glm::max(glm::max(halfExtents.x, halfExtents.y), halfExtents.z);
            glm::vec3 localBoxMin = localCenter - glm::vec3(maxHalfExt);
            glm::vec3 localBoxMax = localCenter + glm::vec3(maxHalfExt);

            // Convert to grid coords for clamping
            glm::ivec3 gridMin = grid->WorldToVoxel(localBoxMin);
            glm::ivec3 gridMax = grid->WorldToVoxel(localBoxMax);

            int w = grid->size.x;
            int h = grid->size.y;
            int d = grid->size.z;
            int xMin = glm::max(0, gridMin.x - 1);
            int yMin = glm::max(0, gridMin.y - 1);
            int zMin = glm::max(0, gridMin.z - 1);
            int xMax = glm::min(w - 1, gridMax.x + 1);
            int yMax = glm::min(h - 1, gridMax.y + 1);
            int zMax = glm::min(d - 1, gridMax.z + 1);

            VoxelMaterialType matType = VoxelMaterialType::Stone;
            if (!dvComp.SubstanceOverride.empty())
                matType = VoxelSubstanceDatabase::ResolveSubstanceName(dvComp.SubstanceOverride);
            std::string substanceName = VoxelMaterialTypeToString(matType);

            for (int x = xMin; x <= xMax; ++x)
            for (int y = yMin; y <= yMax; ++y)
            for (int z = zMin; z <= zMax; ++z)
            {
                if (!grid->IsFilled(x, y, z))
                    continue;

                glm::vec3 localPos = grid->VoxelCenterToWorld(glm::ivec3(x, y, z));
                glm::vec3 worldPos = glm::vec3(worldMatrix * glm::vec4(localPos, 1.0f));

                // Check if world position is inside the query box
                if (worldPos.x < boxMin.x || worldPos.x > boxMax.x ||
                    worldPos.y < boxMin.y || worldPos.y > boxMax.y ||
                    worldPos.z < boxMin.z || worldPos.z > boxMax.z)
                    continue;

                glm::ivec3 coord(x, y, z);
                VoxelHit hit;
                hit.EntityUUID       = uuid;
                hit.WorldPos         = worldPos;
                hit.GridCoord        = coord;
                hit.SubstanceName    = substanceName;
                hit.NormalizedHealth = GetNormalizedHealth(uuid, coord);
                hits.push_back(hit);
            }
        }

        return hits;
    }

    // =========================================================================
    // State read helpers
    // =========================================================================
    float VoxelDestructionSystem::GetNormalizedHealth(uint64_t entityUUID, glm::ivec3 gridCoord)
    {
        auto it = s_EntityStates.find(entityUUID);
        if (it == s_EntityStates.end())
            return 1.0f; // undamaged

        const auto& dmMap = it->second.DamageMap;
        auto hit = dmMap.find(gridCoord);
        if (hit == dmMap.end())
            return 1.0f; // undamaged

        if (hit->second.MaxHealth <= 0.0f)
            return 0.0f;

        return hit->second.CurrentHealth / hit->second.MaxHealth;
    }

    bool VoxelDestructionSystem::IsVoxelDestroyed(uint64_t entityUUID, glm::ivec3 gridCoord)
    {
        auto it = s_EntityStates.find(entityUUID);
        if (it == s_EntityStates.end())
            return false;

        if (!it->second.GridInitialized)
            return false;

        return !it->second.ModifiedGrid.IsFilled(gridCoord.x, gridCoord.y, gridCoord.z);
    }

    std::string VoxelDestructionSystem::GetSubstanceName(uint64_t entityUUID,
                                                           glm::ivec3 /*gridCoord*/,
                                                           Scene* scene)
    {
        if (!scene) return "Stone";

        Entity entity = scene->GetEntityByUUID(UUID(entityUUID));
        if (!entity) return "Stone";
        if (!entity.HasComponent<DestructibleVoxelComponent>()) return "Stone";

        auto& dvComp = entity.GetComponent<DestructibleVoxelComponent>();
        if (!dvComp.SubstanceOverride.empty())
            return dvComp.SubstanceOverride;

        return VoxelMaterialTypeToString(VoxelMaterialType::Stone);
    }
}
