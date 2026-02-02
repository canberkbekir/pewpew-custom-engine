# Voxel ECS Implementation Plan (Teardown-style)

## Overview
Implement entt-based ECS for Teardown-style voxel destruction game with thousands of entities.

---

## File Structure

```
PewPew/
├── vendor/entt/                    # NEW: Header-only library
│   └── entt.hpp
│
├── src/PewPew/
│   ├── Scene/                      # NEW
│   │   ├── Scene.h/cpp             # Owns entt::registry
│   │   ├── Entity.h/cpp            # Wrapper class
│   │   └── SceneSerializer.h/cpp   # Save/Load
│   │
│   ├── Components/                 # NEW
│   │   ├── Components.h            # Aggregate header
│   │   ├── CoreComponents.h        # IDComponent, TagComponent
│   │   ├── TransformComponent.h
│   │   ├── MeshRendererComponent.h
│   │   ├── VoxelDataComponent.h    # Critical for destruction
│   │   ├── RigidBodyComponent.h
│   │   ├── ColliderComponent.h
│   │   └── DestructibleComponent.h
│   │
│   └── Systems/                    # NEW
│       ├── RenderSystem.h/cpp
│       ├── PhysicsSystem.h/cpp
│       ├── VoxelSystem.h/cpp
│       └── DestructionSystem.h/cpp
```

---

## Core Classes

### Scene (owns entt::registry)
```cpp
// PewPew/src/PewPew/Scene/Scene.h

#pragma once

#include <entt.hpp>
#include "PewPew/Core/Core.h"
#include "PewPew/Core/UUID.h"
#include "PewPew/Core/Timestep.h"

namespace PewPew {

    class Entity;

    class Scene
    {
    public:
        Scene() = default;
        ~Scene() = default;

        Entity CreateEntity(const std::string& name = "Entity");
        Entity CreateEntityWithUUID(UUID uuid, const std::string& name);
        void DestroyEntity(Entity entity);

        Entity GetEntityByUUID(UUID uuid);  // O(1) lookup
        bool EntityExists(UUID uuid) const;

        void OnUpdate(Timestep ts);
        void OnRender();

        template<typename... Components>
        auto GetEntitiesWith() { return m_Registry.view<Components...>(); }

        entt::registry& GetRegistry() { return m_Registry; }

    private:
        void ProcessDeferredDestroys();

        entt::registry m_Registry;
        std::unordered_map<UUID, entt::entity> m_EntityMap;  // UUID → entt
        std::vector<entt::entity> m_DeferredDestroys;
    };

}
```

### Entity (wrapper for convenient API)
```cpp
// PewPew/src/PewPew/Scene/Entity.h

#pragma once

#include <entt.hpp>
#include "Scene.h"
#include "PewPew/Core/UUID.h"

namespace PewPew {

    class Entity
    {
    public:
        Entity() = default;
        Entity(entt::entity handle, Scene* scene);
        Entity(const Entity& other) = default;

        template<typename T, typename... Args>
        T& AddComponent(Args&&... args)
        {
            PEW_CORE_ASSERT(!HasComponent<T>(), "Entity already has component!");
            return m_Scene->GetRegistry().emplace<T>(m_Handle, std::forward<Args>(args)...);
        }

        template<typename T>
        T& GetComponent()
        {
            PEW_CORE_ASSERT(HasComponent<T>(), "Entity does not have component!");
            return m_Scene->GetRegistry().get<T>(m_Handle);
        }

        template<typename T>
        const T& GetComponent() const
        {
            PEW_CORE_ASSERT(HasComponent<T>(), "Entity does not have component!");
            return m_Scene->GetRegistry().get<T>(m_Handle);
        }

        template<typename T>
        bool HasComponent() const
        {
            return m_Scene->GetRegistry().all_of<T>(m_Handle);
        }

        template<typename T>
        void RemoveComponent()
        {
            PEW_CORE_ASSERT(HasComponent<T>(), "Entity does not have component!");
            m_Scene->GetRegistry().remove<T>(m_Handle);
        }

        UUID GetUUID() const;
        const std::string& GetName() const;

        operator bool() const { return m_Handle != entt::null; }
        operator entt::entity() const { return m_Handle; }
        operator uint32_t() const { return (uint32_t)m_Handle; }

        bool operator==(const Entity& other) const
        {
            return m_Handle == other.m_Handle && m_Scene == other.m_Scene;
        }

        bool operator!=(const Entity& other) const
        {
            return !(*this == other);
        }

    private:
        entt::entity m_Handle{ entt::null };
        Scene* m_Scene = nullptr;
    };

}
```

---

## Components

### UUID Class (add to Core if not exists)
```cpp
// PewPew/src/PewPew/Core/UUID.h

#pragma once

#include <cstdint>
#include <functional>

namespace PewPew {

    class UUID
    {
    public:
        UUID();
        UUID(uint64_t uuid);
        UUID(const UUID&) = default;

        operator uint64_t() const { return m_UUID; }

    private:
        uint64_t m_UUID;
    };

}

namespace std {
    template<>
    struct hash<PewPew::UUID>
    {
        size_t operator()(const PewPew::UUID& uuid) const
        {
            return hash<uint64_t>()((uint64_t)uuid);
        }
    };
}
```

```cpp
// PewPew/src/PewPew/Core/UUID.cpp

#include "pewpch.h"
#include "UUID.h"

#include <random>

namespace PewPew {

    static std::random_device s_RandomDevice;
    static std::mt19937_64 s_Engine(s_RandomDevice());
    static std::uniform_int_distribution<uint64_t> s_UniformDistribution;

    UUID::UUID()
        : m_UUID(s_UniformDistribution(s_Engine))
    {
    }

    UUID::UUID(uint64_t uuid)
        : m_UUID(uuid)
    {
    }

}
```

### IDComponent & TagComponent (every entity has these)
```cpp
// PewPew/src/PewPew/Components/CoreComponents.h

#pragma once

#include "PewPew/Core/UUID.h"
#include <string>

namespace PewPew {

    struct IDComponent
    {
        UUID ID;

        IDComponent() = default;
        IDComponent(const IDComponent&) = default;
        IDComponent(UUID id) : ID(id) {}
    };

    struct TagComponent
    {
        std::string Tag;

        TagComponent() = default;
        TagComponent(const TagComponent&) = default;
        TagComponent(const std::string& tag) : Tag(tag) {}
    };

}
```

### TransformComponent
```cpp
// PewPew/src/PewPew/Components/TransformComponent.h

#pragma once

#include "PewPew/Math/CoreMath.h"
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

namespace PewPew {

    struct TransformComponent
    {
        Vector3 Position = { 0.0f, 0.0f, 0.0f };
        Vector3 Rotation = { 0.0f, 0.0f, 0.0f };  // Euler angles in radians
        Vector3 Scale = { 1.0f, 1.0f, 1.0f };

        TransformComponent() = default;
        TransformComponent(const TransformComponent&) = default;
        TransformComponent(const Vector3& position)
            : Position(position) {}

        Mat4 GetTransform() const
        {
            glm::mat4 rotation = glm::toMat4(glm::quat(Rotation));

            return glm::translate(glm::mat4(1.0f), Position)
                * rotation
                * glm::scale(glm::mat4(1.0f), Scale);
        }
    };

}
```

### MeshRendererComponent
```cpp
// PewPew/src/PewPew/Components/MeshRendererComponent.h

#pragma once

#include "PewPew/Core/Core.h"
#include "PewPew/Renderer/Resources/Mesh.h"
#include "PewPew/Renderer/Resources/Material.h"
#include "PewPew/Renderer/Resources/Shader.h"

namespace PewPew {

    struct MeshRendererComponent
    {
        Ref<Mesh> MeshAsset;
        Ref<Material> MaterialAsset;
        Ref<Shader> ShaderAsset;
        bool Visible = true;

        MeshRendererComponent() = default;
        MeshRendererComponent(const MeshRendererComponent&) = default;
    };

}
```

### VoxelDataComponent (critical for destruction)
```cpp
// PewPew/src/PewPew/Components/VoxelDataComponent.h

#pragma once

#include "PewPew/Core/Core.h"
#include "PewPew/Math/CoreMath.h"
#include "PewPew/Renderer/Resources/Mesh.h"
#include <vector>

namespace PewPew {

    // Compact voxel grid using bit-packing
    // Each voxel is 1 bit (present/absent) + optional material index
    struct CompactVoxelGrid
    {
        uint32_t SizeX = 0;
        uint32_t SizeY = 0;
        uint32_t SizeZ = 0;
        std::vector<uint8_t> Data;         // Bit-packed presence data
        std::vector<uint8_t> Materials;    // Material index per voxel (if needed)

        bool GetVoxel(uint32_t x, uint32_t y, uint32_t z) const
        {
            if (x >= SizeX || y >= SizeY || z >= SizeZ) return false;
            uint32_t index = x + y * SizeX + z * SizeX * SizeY;
            uint32_t byteIndex = index / 8;
            uint32_t bitIndex = index % 8;
            return (Data[byteIndex] >> bitIndex) & 1;
        }

        void SetVoxel(uint32_t x, uint32_t y, uint32_t z, bool value)
        {
            if (x >= SizeX || y >= SizeY || z >= SizeZ) return;
            uint32_t index = x + y * SizeX + z * SizeX * SizeY;
            uint32_t byteIndex = index / 8;
            uint32_t bitIndex = index % 8;
            if (value)
                Data[byteIndex] |= (1 << bitIndex);
            else
                Data[byteIndex] &= ~(1 << bitIndex);
        }

        void Resize(uint32_t x, uint32_t y, uint32_t z)
        {
            SizeX = x;
            SizeY = y;
            SizeZ = z;
            uint32_t totalBits = x * y * z;
            uint32_t totalBytes = (totalBits + 7) / 8;
            Data.resize(totalBytes, 0);
            Materials.resize(totalBits, 0);
        }

        uint32_t CountVoxels() const
        {
            uint32_t count = 0;
            for (uint32_t z = 0; z < SizeZ; z++)
                for (uint32_t y = 0; y < SizeY; y++)
                    for (uint32_t x = 0; x < SizeX; x++)
                        if (GetVoxel(x, y, z)) count++;
            return count;
        }
    };

    struct VoxelDataComponent
    {
        CompactVoxelGrid Grid;
        Ref<Mesh> CachedMesh;       // Regenerated when voxels change
        bool MeshDirty = true;
        bool IsFragment = false;    // True if debris from destruction
        float TotalMass = 1.0f;
        Vector3 CenterOfMass = { 0.0f, 0.0f, 0.0f };
        float VoxelSize = 0.1f;     // Size of each voxel in world units

        VoxelDataComponent() = default;
        VoxelDataComponent(const VoxelDataComponent&) = default;
    };

}
```

### RigidBodyComponent
```cpp
// PewPew/src/PewPew/Components/RigidBodyComponent.h

#pragma once

#include "PewPew/Math/CoreMath.h"

namespace PewPew {

    enum class RigidBodyType
    {
        Static = 0,     // Never moves
        Dynamic,        // Affected by physics
        Kinematic       // Moves via script, not physics
    };

    struct RigidBodyComponent
    {
        RigidBodyType Type = RigidBodyType::Static;
        float Mass = 1.0f;
        float LinearDamping = 0.01f;
        float AngularDamping = 0.05f;
        Vector3 LinearVelocity = { 0.0f, 0.0f, 0.0f };
        Vector3 AngularVelocity = { 0.0f, 0.0f, 0.0f };
        bool IsSleeping = false;    // Optimization for debris at rest
        bool UseGravity = true;

        RigidBodyComponent() = default;
        RigidBodyComponent(const RigidBodyComponent&) = default;
    };

}
```

### ColliderComponent
```cpp
// PewPew/src/PewPew/Components/ColliderComponent.h

#pragma once

#include "PewPew/Math/CoreMath.h"

namespace PewPew {

    enum class ColliderType
    {
        Box = 0,
        Sphere,
        Mesh,       // For complex collision shapes
        Voxel       // Auto-generated from voxel data
    };

    struct ColliderComponent
    {
        ColliderType Type = ColliderType::Box;
        Vector3 Size = { 1.0f, 1.0f, 1.0f };    // Box dimensions or sphere radius in x
        Vector3 Offset = { 0.0f, 0.0f, 0.0f };  // Local offset from transform
        bool IsTrigger = false;                  // Trigger vs solid collider

        ColliderComponent() = default;
        ColliderComponent(const ColliderComponent&) = default;
    };

}
```

### DestructibleComponent
```cpp
// PewPew/src/PewPew/Components/DestructibleComponent.h

#pragma once

#include <cstdint>

namespace PewPew {

    enum class DestructionMode
    {
        Voxel = 0,      // Individual voxel removal
        Chunks,         // Break into connected chunks
        Shatter         // Explode into small fragments
    };

    struct DestructibleComponent
    {
        DestructionMode Mode = DestructionMode::Chunks;
        float Health = 100.0f;
        float MaxHealth = 100.0f;
        uint32_t MinChunkSize = 2;      // Minimum voxels per chunk
        uint32_t MaxChunkSize = 16;     // Maximum voxels per chunk
        float FragmentLifetime = 10.0f; // Seconds before auto-despawn (0 = never)
        float FragmentAge = 0.0f;       // Current age if IsFragment
        bool Indestructible = false;    // Ignore all damage

        DestructibleComponent() = default;
        DestructibleComponent(const DestructibleComponent&) = default;
    };

}
```

### Components Aggregate Header
```cpp
// PewPew/src/PewPew/Components/Components.h

#pragma once

#include "CoreComponents.h"
#include "TransformComponent.h"
#include "MeshRendererComponent.h"
#include "VoxelDataComponent.h"
#include "RigidBodyComponent.h"
#include "ColliderComponent.h"
#include "DestructibleComponent.h"
```

---

## Systems

### RenderSystem
```cpp
// PewPew/src/PewPew/Systems/RenderSystem.h

#pragma once

#include "PewPew/Scene/Scene.h"
#include "PewPew/Renderer/Camera/Camera.h"

namespace PewPew {

    class RenderSystem
    {
    public:
        static void OnRender(Scene& scene, const Camera& camera, const Vector3& cameraPosition);
    };

}
```

```cpp
// PewPew/src/PewPew/Systems/RenderSystem.cpp

#include "pewpch.h"
#include "RenderSystem.h"
#include "PewPew/Components/Components.h"
#include "PewPew/Renderer/Core/Renderer3D.h"

namespace PewPew {

    void RenderSystem::OnRender(Scene& scene, const Camera& camera, const Vector3& cameraPosition)
    {
        Renderer3D::BeginScene(camera, cameraPosition);

        // Render mesh entities
        auto meshView = scene.GetEntitiesWith<TransformComponent, MeshRendererComponent>();
        for (auto entity : meshView)
        {
            auto& [transform, meshRenderer] = meshView.get<TransformComponent, MeshRendererComponent>(entity);

            if (!meshRenderer.Visible || !meshRenderer.MeshAsset)
                continue;

            Mat4 transformMatrix = transform.GetTransform();
            Renderer3D::Submit(
                meshRenderer.ShaderAsset,
                meshRenderer.MaterialAsset,
                meshRenderer.MeshAsset,
                transformMatrix
            );
        }

        // Render voxel entities (use cached mesh)
        auto voxelView = scene.GetEntitiesWith<TransformComponent, VoxelDataComponent>();
        for (auto entity : voxelView)
        {
            auto& [transform, voxelData] = voxelView.get<TransformComponent, VoxelDataComponent>(entity);

            if (!voxelData.CachedMesh)
                continue;

            Mat4 transformMatrix = transform.GetTransform();
            // Use default shader/material for voxels or add MeshRendererComponent
            Renderer3D::Submit(nullptr, nullptr, voxelData.CachedMesh, transformMatrix);
        }

        Renderer3D::EndScene();
    }

}
```

### PhysicsSystem
```cpp
// PewPew/src/PewPew/Systems/PhysicsSystem.h

#pragma once

#include "PewPew/Scene/Scene.h"
#include "PewPew/Core/Timestep.h"

namespace PewPew {

    class PhysicsSystem
    {
    public:
        static void OnUpdate(Scene& scene, Timestep ts);

        static void SetGravity(const Vector3& gravity) { s_Gravity = gravity; }
        static Vector3 GetGravity() { return s_Gravity; }

    private:
        static void IntegrateVelocities(Scene& scene, Timestep ts);
        static void DetectCollisions(Scene& scene);
        static void ResolveCollisions(Scene& scene);

        inline static Vector3 s_Gravity = { 0.0f, -9.81f, 0.0f };
    };

}
```

```cpp
// PewPew/src/PewPew/Systems/PhysicsSystem.cpp

#include "pewpch.h"
#include "PhysicsSystem.h"
#include "PewPew/Components/Components.h"

namespace PewPew {

    void PhysicsSystem::OnUpdate(Scene& scene, Timestep ts)
    {
        IntegrateVelocities(scene, ts);
        DetectCollisions(scene);
        ResolveCollisions(scene);
    }

    void PhysicsSystem::IntegrateVelocities(Scene& scene, Timestep ts)
    {
        auto view = scene.GetEntitiesWith<TransformComponent, RigidBodyComponent>();

        for (auto entity : view)
        {
            auto& [transform, rb] = view.get<TransformComponent, RigidBodyComponent>(entity);

            if (rb.Type != RigidBodyType::Dynamic || rb.IsSleeping)
                continue;

            // Apply gravity
            if (rb.UseGravity)
                rb.LinearVelocity += s_Gravity * ts.GetSeconds();

            // Apply damping
            rb.LinearVelocity *= (1.0f - rb.LinearDamping);
            rb.AngularVelocity *= (1.0f - rb.AngularDamping);

            // Integrate position
            transform.Position += rb.LinearVelocity * ts.GetSeconds();
            transform.Rotation += rb.AngularVelocity * ts.GetSeconds();

            // Check for sleep (velocity below threshold)
            float linearSpeedSq = glm::dot(rb.LinearVelocity, rb.LinearVelocity);
            float angularSpeedSq = glm::dot(rb.AngularVelocity, rb.AngularVelocity);
            if (linearSpeedSq < 0.001f && angularSpeedSq < 0.001f)
            {
                rb.LinearVelocity = { 0.0f, 0.0f, 0.0f };
                rb.AngularVelocity = { 0.0f, 0.0f, 0.0f };
                rb.IsSleeping = true;
            }
        }
    }

    void PhysicsSystem::DetectCollisions(Scene& scene)
    {
        // TODO: Implement broad-phase (spatial hash) and narrow-phase collision
        // For now, just ground plane collision
        auto view = scene.GetEntitiesWith<TransformComponent, RigidBodyComponent, ColliderComponent>();

        for (auto entity : view)
        {
            auto& [transform, rb, collider] = view.get<TransformComponent, RigidBodyComponent, ColliderComponent>(entity);

            if (rb.Type != RigidBodyType::Dynamic)
                continue;

            // Simple ground plane at y=0
            float bottomY = transform.Position.y - collider.Size.y * 0.5f;
            if (bottomY < 0.0f)
            {
                transform.Position.y = collider.Size.y * 0.5f;
                rb.LinearVelocity.y = -rb.LinearVelocity.y * 0.3f; // Bounce with energy loss
                rb.IsSleeping = false;
            }
        }
    }

    void PhysicsSystem::ResolveCollisions(Scene& scene)
    {
        // TODO: Implement collision response
    }

}
```

### VoxelSystem
```cpp
// PewPew/src/PewPew/Systems/VoxelSystem.h

#pragma once

#include "PewPew/Scene/Scene.h"
#include "PewPew/Components/VoxelDataComponent.h"

namespace PewPew {

    class VoxelSystem
    {
    public:
        static void RegenerateDirtyMeshes(Scene& scene);
        static Ref<Mesh> GenerateMeshFromGrid(const CompactVoxelGrid& grid, float voxelSize);

        // Utility functions
        static void RemoveVoxel(VoxelDataComponent& voxelData, uint32_t x, uint32_t y, uint32_t z);
        static void RemoveVoxelsInRadius(VoxelDataComponent& voxelData, const Vector3& center, float radius);
    };

}
```

```cpp
// PewPew/src/PewPew/Systems/VoxelSystem.cpp

#include "pewpch.h"
#include "VoxelSystem.h"
#include "PewPew/Components/Components.h"

namespace PewPew {

    void VoxelSystem::RegenerateDirtyMeshes(Scene& scene)
    {
        auto view = scene.GetEntitiesWith<VoxelDataComponent>();

        for (auto entity : view)
        {
            auto& voxelData = view.get<VoxelDataComponent>(entity);

            if (!voxelData.MeshDirty)
                continue;

            voxelData.CachedMesh = GenerateMeshFromGrid(voxelData.Grid, voxelData.VoxelSize);
            voxelData.MeshDirty = false;

            // Recalculate center of mass
            Vector3 com = { 0.0f, 0.0f, 0.0f };
            uint32_t count = 0;
            for (uint32_t z = 0; z < voxelData.Grid.SizeZ; z++)
            {
                for (uint32_t y = 0; y < voxelData.Grid.SizeY; y++)
                {
                    for (uint32_t x = 0; x < voxelData.Grid.SizeX; x++)
                    {
                        if (voxelData.Grid.GetVoxel(x, y, z))
                        {
                            com += Vector3(x, y, z) * voxelData.VoxelSize;
                            count++;
                        }
                    }
                }
            }
            if (count > 0)
            {
                voxelData.CenterOfMass = com / (float)count;
                voxelData.TotalMass = count * 0.1f; // Mass per voxel
            }
        }
    }

    Ref<Mesh> VoxelSystem::GenerateMeshFromGrid(const CompactVoxelGrid& grid, float voxelSize)
    {
        // Greedy meshing or simple cube-per-voxel approach
        // This is a simplified version - use greedy meshing for optimization

        std::vector<float> vertices;
        std::vector<uint32_t> indices;
        uint32_t indexOffset = 0;

        for (uint32_t z = 0; z < grid.SizeZ; z++)
        {
            for (uint32_t y = 0; y < grid.SizeY; y++)
            {
                for (uint32_t x = 0; x < grid.SizeX; x++)
                {
                    if (!grid.GetVoxel(x, y, z))
                        continue;

                    Vector3 pos = Vector3(x, y, z) * voxelSize;
                    float s = voxelSize * 0.5f;

                    // Check each face - only add if neighbor is empty
                    // +X face
                    if (x == grid.SizeX - 1 || !grid.GetVoxel(x + 1, y, z))
                    {
                        // Add vertices and indices for +X face
                        // ... (full implementation would add all face vertices)
                    }
                    // -X, +Y, -Y, +Z, -Z faces similarly...

                    // Simplified: add cube (full mesh generation would be more complex)
                    // TODO: Implement proper greedy meshing
                }
            }
        }

        // Create mesh from vertices/indices
        // return Mesh::Create(vertices, indices);
        return nullptr; // Placeholder
    }

    void VoxelSystem::RemoveVoxel(VoxelDataComponent& voxelData, uint32_t x, uint32_t y, uint32_t z)
    {
        voxelData.Grid.SetVoxel(x, y, z, false);
        voxelData.MeshDirty = true;
    }

    void VoxelSystem::RemoveVoxelsInRadius(VoxelDataComponent& voxelData, const Vector3& center, float radius)
    {
        float radiusSq = radius * radius;
        float voxelSize = voxelData.VoxelSize;

        for (uint32_t z = 0; z < voxelData.Grid.SizeZ; z++)
        {
            for (uint32_t y = 0; y < voxelData.Grid.SizeY; y++)
            {
                for (uint32_t x = 0; x < voxelData.Grid.SizeX; x++)
                {
                    Vector3 voxelPos = Vector3(x, y, z) * voxelSize;
                    Vector3 diff = voxelPos - center;
                    if (glm::dot(diff, diff) <= radiusSq)
                    {
                        voxelData.Grid.SetVoxel(x, y, z, false);
                    }
                }
            }
        }
        voxelData.MeshDirty = true;
    }

}
```

### DestructionSystem
```cpp
// PewPew/src/PewPew/Systems/DestructionSystem.h

#pragma once

#include "PewPew/Scene/Scene.h"
#include "PewPew/Core/Timestep.h"

namespace PewPew {

    struct DamageInfo
    {
        Vector3 Point;          // World position of damage
        Vector3 Direction;      // Direction of force
        float Damage;           // Amount of damage
        float Radius;           // Radius of effect (0 = point damage)
        float Force;            // Explosion force for fragments
    };

    class DestructionSystem
    {
    public:
        static void OnUpdate(Scene& scene, Timestep ts);
        static void CleanupExpiredFragments(Scene& scene);

        // Apply damage to an entity
        static void ApplyDamage(Scene& scene, Entity entity, const DamageInfo& damage);

    private:
        // Flood-fill to find connected voxel chunks
        static std::vector<CompactVoxelGrid> FindConnectedChunks(const CompactVoxelGrid& grid);

        // Create fragment entities from chunks
        static void SpawnFragments(Scene& scene, Entity original,
                                   const std::vector<CompactVoxelGrid>& chunks,
                                   const DamageInfo& damage);
    };

}
```

```cpp
// PewPew/src/PewPew/Systems/DestructionSystem.cpp

#include "pewpch.h"
#include "DestructionSystem.h"
#include "PewPew/Components/Components.h"
#include "VoxelSystem.h"
#include <queue>

namespace PewPew {

    void DestructionSystem::OnUpdate(Scene& scene, Timestep ts)
    {
        // Update fragment ages
        auto view = scene.GetEntitiesWith<DestructibleComponent, VoxelDataComponent>();

        for (auto entity : view)
        {
            auto& destructible = view.get<DestructibleComponent>(entity);
            auto& voxelData = view.get<VoxelDataComponent>(entity);

            if (voxelData.IsFragment)
            {
                destructible.FragmentAge += ts.GetSeconds();
            }
        }
    }

    void DestructionSystem::CleanupExpiredFragments(Scene& scene)
    {
        auto view = scene.GetEntitiesWith<DestructibleComponent, VoxelDataComponent>();
        std::vector<Entity> toDestroy;

        for (auto entityHandle : view)
        {
            auto& destructible = view.get<DestructibleComponent>(entityHandle);
            auto& voxelData = view.get<VoxelDataComponent>(entityHandle);

            if (voxelData.IsFragment &&
                destructible.FragmentLifetime > 0.0f &&
                destructible.FragmentAge >= destructible.FragmentLifetime)
            {
                toDestroy.push_back(Entity(entityHandle, &scene));
            }
        }

        for (auto& entity : toDestroy)
        {
            scene.DestroyEntity(entity);
        }
    }

    void DestructionSystem::ApplyDamage(Scene& scene, Entity entity, const DamageInfo& damage)
    {
        if (!entity.HasComponent<DestructibleComponent>() || !entity.HasComponent<VoxelDataComponent>())
            return;

        auto& destructible = entity.GetComponent<DestructibleComponent>();
        auto& voxelData = entity.GetComponent<VoxelDataComponent>();
        auto& transform = entity.GetComponent<TransformComponent>();

        if (destructible.Indestructible)
            return;

        // Apply damage
        destructible.Health -= damage.Damage;

        // Remove voxels in radius (convert world to local space)
        Vector3 localPoint = damage.Point - transform.Position;
        VoxelSystem::RemoveVoxelsInRadius(voxelData, localPoint, damage.Radius);

        // Check if destroyed
        if (destructible.Health <= 0.0f || destructible.Mode == DestructionMode::Shatter)
        {
            // Find connected chunks
            auto chunks = FindConnectedChunks(voxelData.Grid);

            if (chunks.size() > 1 || destructible.Mode == DestructionMode::Shatter)
            {
                // Spawn fragments and destroy original
                SpawnFragments(scene, entity, chunks, damage);
                scene.DestroyEntity(entity);
            }
        }
    }

    std::vector<CompactVoxelGrid> DestructionSystem::FindConnectedChunks(const CompactVoxelGrid& grid)
    {
        std::vector<CompactVoxelGrid> chunks;

        // Track visited voxels
        std::vector<bool> visited(grid.SizeX * grid.SizeY * grid.SizeZ, false);

        auto getIndex = [&](uint32_t x, uint32_t y, uint32_t z) {
            return x + y * grid.SizeX + z * grid.SizeX * grid.SizeY;
        };

        // 6-connected neighbors
        const int dx[] = { 1, -1, 0, 0, 0, 0 };
        const int dy[] = { 0, 0, 1, -1, 0, 0 };
        const int dz[] = { 0, 0, 0, 0, 1, -1 };

        // Find all connected components using flood-fill
        for (uint32_t z = 0; z < grid.SizeZ; z++)
        {
            for (uint32_t y = 0; y < grid.SizeY; y++)
            {
                for (uint32_t x = 0; x < grid.SizeX; x++)
                {
                    if (!grid.GetVoxel(x, y, z) || visited[getIndex(x, y, z)])
                        continue;

                    // Start new chunk
                    CompactVoxelGrid chunk;
                    chunk.Resize(grid.SizeX, grid.SizeY, grid.SizeZ);

                    std::queue<std::tuple<uint32_t, uint32_t, uint32_t>> queue;
                    queue.push({ x, y, z });
                    visited[getIndex(x, y, z)] = true;

                    while (!queue.empty())
                    {
                        auto [cx, cy, cz] = queue.front();
                        queue.pop();

                        chunk.SetVoxel(cx, cy, cz, true);

                        // Check neighbors
                        for (int i = 0; i < 6; i++)
                        {
                            int nx = cx + dx[i];
                            int ny = cy + dy[i];
                            int nz = cz + dz[i];

                            if (nx < 0 || nx >= (int)grid.SizeX ||
                                ny < 0 || ny >= (int)grid.SizeY ||
                                nz < 0 || nz >= (int)grid.SizeZ)
                                continue;

                            uint32_t ni = getIndex(nx, ny, nz);
                            if (!visited[ni] && grid.GetVoxel(nx, ny, nz))
                            {
                                visited[ni] = true;
                                queue.push({ (uint32_t)nx, (uint32_t)ny, (uint32_t)nz });
                            }
                        }
                    }

                    chunks.push_back(std::move(chunk));
                }
            }
        }

        return chunks;
    }

    void DestructionSystem::SpawnFragments(Scene& scene, Entity original,
                                           const std::vector<CompactVoxelGrid>& chunks,
                                           const DamageInfo& damage)
    {
        auto& originalTransform = original.GetComponent<TransformComponent>();
        auto& originalVoxel = original.GetComponent<VoxelDataComponent>();
        auto& originalDestructible = original.GetComponent<DestructibleComponent>();

        for (size_t i = 0; i < chunks.size(); i++)
        {
            const auto& chunk = chunks[i];
            uint32_t voxelCount = chunk.CountVoxels();

            // Skip tiny fragments
            if (voxelCount < originalDestructible.MinChunkSize)
                continue;

            // Create fragment entity
            Entity fragment = scene.CreateEntity("Fragment_" + std::to_string(i));

            // Transform
            auto& transform = fragment.AddComponent<TransformComponent>();
            transform.Position = originalTransform.Position;
            transform.Rotation = originalTransform.Rotation;
            transform.Scale = originalTransform.Scale;

            // Voxel data
            auto& voxelData = fragment.AddComponent<VoxelDataComponent>();
            voxelData.Grid = chunk;
            voxelData.VoxelSize = originalVoxel.VoxelSize;
            voxelData.IsFragment = true;
            voxelData.MeshDirty = true;

            // Physics
            auto& rb = fragment.AddComponent<RigidBodyComponent>();
            rb.Type = RigidBodyType::Dynamic;
            rb.Mass = voxelCount * 0.1f;

            // Apply explosion velocity
            Vector3 fragmentCenter = voxelData.CenterOfMass * voxelData.VoxelSize + transform.Position;
            Vector3 toFragment = glm::normalize(fragmentCenter - damage.Point);
            rb.LinearVelocity = toFragment * damage.Force + damage.Direction * damage.Force * 0.5f;
            rb.AngularVelocity = Vector3(
                (rand() / (float)RAND_MAX - 0.5f) * 5.0f,
                (rand() / (float)RAND_MAX - 0.5f) * 5.0f,
                (rand() / (float)RAND_MAX - 0.5f) * 5.0f
            );

            // Collider
            auto& collider = fragment.AddComponent<ColliderComponent>();
            collider.Type = ColliderType::Box;
            collider.Size = Vector3(chunk.SizeX, chunk.SizeY, chunk.SizeZ) * voxelData.VoxelSize;

            // Destructible (fragments can be further destroyed)
            auto& destructible = fragment.AddComponent<DestructibleComponent>();
            destructible.Mode = DestructionMode::Voxel;
            destructible.FragmentLifetime = originalDestructible.FragmentLifetime;
            destructible.MinChunkSize = originalDestructible.MinChunkSize;
        }
    }

}
```

---

## Scene Implementation

```cpp
// PewPew/src/PewPew/Scene/Scene.cpp

#include "pewpch.h"
#include "Scene.h"
#include "Entity.h"
#include "PewPew/Components/Components.h"
#include "PewPew/Systems/RenderSystem.h"
#include "PewPew/Systems/PhysicsSystem.h"
#include "PewPew/Systems/VoxelSystem.h"
#include "PewPew/Systems/DestructionSystem.h"

namespace PewPew {

    Entity Scene::CreateEntity(const std::string& name)
    {
        return CreateEntityWithUUID(UUID(), name);
    }

    Entity Scene::CreateEntityWithUUID(UUID uuid, const std::string& name)
    {
        Entity entity = { m_Registry.create(), this };
        entity.AddComponent<IDComponent>(uuid);
        entity.AddComponent<TagComponent>(name.empty() ? "Entity" : name);
        entity.AddComponent<TransformComponent>();

        m_EntityMap[uuid] = entity;
        return entity;
    }

    void Scene::DestroyEntity(Entity entity)
    {
        m_EntityMap.erase(entity.GetUUID());
        m_Registry.destroy(entity);
    }

    Entity Scene::GetEntityByUUID(UUID uuid)
    {
        if (m_EntityMap.find(uuid) != m_EntityMap.end())
            return { m_EntityMap.at(uuid), this };
        return {};
    }

    bool Scene::EntityExists(UUID uuid) const
    {
        return m_EntityMap.find(uuid) != m_EntityMap.end();
    }

    void Scene::OnUpdate(Timestep ts)
    {
        // 1. Process deferred destruction
        ProcessDeferredDestroys();

        // 2. Destruction system (spawn fragments)
        DestructionSystem::OnUpdate(*this, ts);

        // 3. Physics system (gravity, collision)
        PhysicsSystem::OnUpdate(*this, ts);

        // 4. Voxel mesh regeneration
        VoxelSystem::RegenerateDirtyMeshes(*this);

        // 5. Fragment cleanup
        DestructionSystem::CleanupExpiredFragments(*this);
    }

    void Scene::OnRender()
    {
        // Rendering is handled by EditorLayer/ViewportPanel which calls RenderSystem
    }

    void Scene::ProcessDeferredDestroys()
    {
        for (auto entity : m_DeferredDestroys)
        {
            auto& id = m_Registry.get<IDComponent>(entity);
            m_EntityMap.erase(id.ID);
            m_Registry.destroy(entity);
        }
        m_DeferredDestroys.clear();
    }

}
```

```cpp
// PewPew/src/PewPew/Scene/Entity.cpp

#include "pewpch.h"
#include "Entity.h"
#include "PewPew/Components/CoreComponents.h"

namespace PewPew {

    Entity::Entity(entt::entity handle, Scene* scene)
        : m_Handle(handle), m_Scene(scene)
    {
    }

    UUID Entity::GetUUID() const
    {
        return GetComponent<IDComponent>().ID;
    }

    const std::string& Entity::GetName() const
    {
        return GetComponent<TagComponent>().Tag;
    }

}
```

---

## Scene Serialization

```cpp
// PewPew/src/PewPew/Scene/SceneSerializer.h

#pragma once

#include "Scene.h"
#include <string>

namespace PewPew {

    class SceneSerializer
    {
    public:
        SceneSerializer(const Ref<Scene>& scene);

        void Serialize(const std::string& filepath);
        bool Deserialize(const std::string& filepath);

        // YAML format
        void SerializeText(const std::string& filepath);
        bool DeserializeText(const std::string& filepath);

    private:
        Ref<Scene> m_Scene;
    };

}
```

---

## Premake Configuration Changes

Add to `premake5.lua`:

```lua
-- In PewPew project includedirs
includedirs {
    -- ... existing includes ...
    "%{wks.location}/PewPew/vendor/entt/include"
}
```

---

## Integration with Editor

### EditorLayer Changes
```cpp
// PewPew-Editor/src/EditorLayer.h additions

#include "PewPew/Scene/Scene.h"

class EditorLayer : public PewPew::Layer
{
    // ... existing code ...

private:
    Ref<Scene> m_ActiveScene;
    Entity m_SelectedEntity;
};
```

### SceneHierarchyPanel
```cpp
// PewPew-Editor/src/Panels/SceneHierarchyPanel.h

#pragma once

#include "PewPew/Scene/Scene.h"
#include "PewPew/Scene/Entity.h"

namespace PewPew {

    class SceneHierarchyPanel
    {
    public:
        SceneHierarchyPanel() = default;
        SceneHierarchyPanel(const Ref<Scene>& scene);

        void SetContext(const Ref<Scene>& scene);
        void OnImGuiRender();

        Entity GetSelectedEntity() const { return m_SelectionContext; }
        void SetSelectedEntity(Entity entity) { m_SelectionContext = entity; }

    private:
        void DrawEntityNode(Entity entity);

        Ref<Scene> m_Context;
        Entity m_SelectionContext;
    };

}
```

### PropertiesPanel (Component Drawing)
```cpp
// PewPew-Editor/src/Panels/PropertiesPanel.h

#pragma once

#include "PewPew/Scene/Entity.h"

namespace PewPew {

    class PropertiesPanel
    {
    public:
        void OnImGuiRender(Entity selectedEntity);

    private:
        void DrawComponents(Entity entity);

        template<typename T>
        void DrawComponent(const std::string& name, Entity entity,
                          std::function<void(T&)> uiFunction);
    };

}
```

---

## Implementation Phases (Checklist)

### Phase 1: Core ECS
- [ ] Download entt (single header) to `PewPew/vendor/entt/include/entt.hpp`
- [ ] Update `premake5.lua` with include path
- [ ] Create `PewPew/src/PewPew/Core/UUID.h` and `.cpp`
- [ ] Create `PewPew/src/PewPew/Scene/Scene.h` and `.cpp`
- [ ] Create `PewPew/src/PewPew/Scene/Entity.h` and `.cpp`
- [ ] Create `PewPew/src/PewPew/Components/CoreComponents.h`
- [ ] Create `PewPew/src/PewPew/Components/TransformComponent.h`
- [ ] Add includes to `PewPew.h`
- [ ] Test: Create entity, add components, verify in debugger

### Phase 2: Rendering Integration
- [ ] Create `PewPew/src/PewPew/Components/MeshRendererComponent.h`
- [ ] Create `PewPew/src/PewPew/Systems/RenderSystem.h` and `.cpp`
- [ ] Modify `EditorLayer` to use `Scene` for rendering
- [ ] Test: Entity with mesh renders in viewport

### Phase 3: Editor Panels
- [ ] Create `SceneHierarchyPanel.h` and `.cpp`
- [ ] Create or update `PropertiesPanel.h` and `.cpp`
- [ ] Add entity creation/deletion UI
- [ ] Add component add/remove UI
- [ ] Test: Full editor workflow

### Phase 4: Voxel System
- [ ] Create `PewPew/src/PewPew/Components/VoxelDataComponent.h`
- [ ] Create `PewPew/src/PewPew/Systems/VoxelSystem.h` and `.cpp`
- [ ] Implement greedy mesh generation
- [ ] Integrate with `VoxelizerAPI`
- [ ] Test: Voxelize mesh, render result

### Phase 5: Physics System
- [ ] Create `PewPew/src/PewPew/Components/RigidBodyComponent.h`
- [ ] Create `PewPew/src/PewPew/Components/ColliderComponent.h`
- [ ] Create `PewPew/src/PewPew/Systems/PhysicsSystem.h` and `.cpp`
- [ ] Implement basic gravity and ground collision
- [ ] Test: Dynamic objects fall and bounce

### Phase 6: Destruction System
- [ ] Create `PewPew/src/PewPew/Components/DestructibleComponent.h`
- [ ] Create `PewPew/src/PewPew/Systems/DestructionSystem.h` and `.cpp`
- [ ] Implement flood-fill chunk detection
- [ ] Implement fragment spawning
- [ ] Test: Damage voxel object, watch it split

### Phase 7: Serialization
- [ ] Create `PewPew/src/PewPew/Scene/SceneSerializer.h` and `.cpp`
- [ ] Add yaml-cpp to vendor (or use JSON)
- [ ] Implement save/load for all components
- [ ] Add File menu items in editor
- [ ] Test: Save scene, reload, verify entities

---

## Key Algorithms Reference

### Greedy Meshing (for VoxelSystem)
```
For each axis (X, Y, Z):
    For each slice perpendicular to axis:
        Mark all visible faces
        While unmarked faces remain:
            Find largest rectangle of same material
            Add quad to mesh
            Mark faces as processed
```

### Flood-Fill Connected Components (for DestructionSystem)
```
visited = empty set
chunks = empty list

for each voxel v:
    if v is solid and v not in visited:
        chunk = new grid
        queue = [v]
        while queue not empty:
            current = queue.pop()
            chunk.add(current)
            visited.add(current)
            for each neighbor n of current:
                if n is solid and n not in visited:
                    queue.push(n)
        chunks.add(chunk)

return chunks
```

---

## Performance Considerations

1. **Spatial Partitioning**: Use octree or spatial hash for collision detection
2. **Mesh Caching**: Only regenerate dirty meshes
3. **Sleep System**: Disable physics for stationary fragments
4. **LOD**: Simplify distant voxel meshes
5. **Chunk Pooling**: Reuse CompactVoxelGrid allocations
6. **Batch Rendering**: Group fragments by material

---

## Verification Checklist

1. [ ] Create entity in editor -> appears in hierarchy
2. [ ] Add components -> shows in properties panel
3. [ ] Create voxel entity from mesh -> renders correctly
4. [ ] Apply damage -> splits into fragments
5. [ ] Fragments fall with gravity -> despawn after lifetime
6. [ ] Save/Load scene -> all entities preserved
7. [ ] Performance: 1000+ entities at 60fps
