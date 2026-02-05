# CBEngine - ECS & Voxel Physics Roadmap

> **Approach:** Custom simple physics (no library) | **Priority:** Voxel ECS first, then physics

---

## Engine Architecture Overview

```
                          +------------------+
                          |   Application    |
                          +--------+---------+
                                   |
                          +--------v---------+
                          |   Scene Manager  |
                          +--------+---------+
                                   |
                    +--------------v---------------+
                    |           Scene              |
                    |   (entt registry + systems)  |
                    +--------------+---------------+
                                   |
            +----------+-----------+-----------+-----------+
            |          |           |           |           |
        +---v---+  +---v---+  +---v----+  +---v---+  +---v----+
        |  ID   |  |  Tag  |  |Transform|  | Mesh  |  | Voxel  |
        +-------+  +-------+  +---+----+  |Renderer|  +--------+
                                   |      +--------+
                               +---v---+
                               |Rigid  |
                               | Body  |
                               +---+---+
                                   |
                               +---v----+
                               |Collider|
                               +--------+
```

---

## What's Done vs What's Needed

```
  COMPONENTS                        SYSTEMS                         INFRASTRUCTURE
  ~~~~~~~~~~                        ~~~~~~~                         ~~~~~~~~~~~~~~

  [##] IDComponent                  [##] TransformSystem            [##] Scene + Entity
  [##] TagComponent                 [  ] PhysicsSystem              [##] SceneSerializer
  [##] TransformComponent           [  ] VoxelSystem                [##] SceneManager
  [##] MeshRendererComponent        [  ] DestructionSystem          [##] Selection
  [  ] VoxelComponent               [  ] RenderSystem               [##] Asset System
  [  ] RigidBodyComponent           [  ] LightSystem                [##] VoxelizerAPI
  [  ] ColliderComponent                                            [##] Renderer3D (PBR)
  [  ] CameraComponent
  [  ] LightComponent               [##] = Done    [  ] = Todo
```

---

## System Update Pipeline

```
  Scene::OnUpdate(dt)
  |
  |  1. ProcessDeferredDestroys()          [##] done
  |     Remove entities queued last frame
  |
  |  2. TransformSystem::OnUpdate()        [##] done
  |     Update world matrices (hierarchy)
  |
  |  3. PhysicsSystem::OnUpdate()          [  ] todo
  |     Gravity -> Integrate -> Collide -> Resolve
  |
  |  4. VoxelSystem::OnUpdate()            [  ] todo
  |     Regenerate dirty voxel meshes
  |
  |  5. DestructionSystem::OnUpdate()      [  ] todo
  |     Break voxels on impact, spawn fragments, cleanup
```

---

## Implementation Roadmap

```
  PHASE 1                  PHASE 2                PHASE 3                PHASE 4
  Voxel ECS                Physics Core           Destruction            Polish
  =========                ============           ===========            ======

  VoxelComponent      -->  RigidBodyComponent -->  DestructionSystem -->  CameraComponent
  VoxelSystem              ColliderComponent       Fragment spawning      LightComponent
  Auto-voxelize on         PhysicsSystem           Fragment cleanup       Serialization
   3D file import          (gravity + AABB)        Voxel removal          Editor UI
```

---

## Phase 1: Voxel ECS Integration

> Connect existing `VoxelizerAPI` into the entity component system.

### VoxelComponent

```
File: CBEngine/src/CBEngine/Components/VoxelComponent.h
```

| Field | Type | Description |
|-------|------|-------------|
| Data | `VoxelMeshData` | Voxel grid + rendered mesh |
| Settings | `VoxelizeSettings` | Grid size, solid/surface |
| SourceMeshUUID | `UUID` | Original mesh to voxelize from |
| SourceTextureUUID | `UUID` | Texture for color sampling |
| Dirty | `bool` | Flag to regenerate mesh |
| AutoVoxelize | `bool` | Voxelize on load |

**Tasks:**
- [ ] Create VoxelComponent struct
- [ ] Add to `Components.h` include list

### VoxelSystem

```
File: CBEngine/src/CBEngine/Systems/VoxelSystem.h/.cpp
```

**Tasks:**
- [ ] On dirty VoxelComponent: re-voxelize from source mesh, regenerate render mesh
- [ ] On initial load: auto-voxelize if `AutoVoxelize == true`
- [ ] API to remove individual voxels by grid coordinate (for destruction later)
- [ ] Wire into `Scene::OnUpdate` (currently commented out at line 94)

### Auto-Voxelize on 3D File Import

When a 3D file (FBX/OBJ/GLTF) is imported via drag-drop or content browser,
automatically voxelize it and create a scene entity with the voxelized mesh.

```
  User drops Model.fbx into Content Browser
        |
        v
  ImportExternalFiles()                         <-- ContentBrowserPanel.cpp
        |
        +-- 1. Copy file to assets/models/
        +-- 2. AssetManager::ImportAsset()      <-- registers UUID + .meta
        +-- 3. NEW: Auto-voxelize pipeline
                |
                v
        +-------+--------+
        | Detect 3D file  |
        | (.fbx/.obj/.gltf)
        +-------+---------+
                |
                v
        +-------+--------+
        | Load mesh via   |
        | Mesh::Load()    |
        +-------+---------+
                |
                v
        +-------+---------+
        | VoxelizerAPI::  |
        | Voxelize()      |
        +-------+---------+
                |
                v
        +-------+-------------------+
        | Create Entity in scene    |
        |  + TagComponent (filename)|
        |  + TransformComponent     |
        |  + VoxelComponent (grid)  |
        |  + MeshRendererComponent  |
        |    (voxelized mesh)       |
        +---------------------------+
```

**Integration points:**

| File | What to change |
|------|----------------|
| `CBEngine-Editor/src/Panels/ContentBrowserPanel.cpp` | Add voxelize step in `ImportExternalFiles()` after `AssetManager::ImportAsset()` for mesh files |
| `CBEngine-Editor/src/Panels/ContentBrowserPanel.h` | Add `VoxelizeSettings m_DefaultVoxelSettings` for editor defaults |

**Tasks:**
- [ ] Detect mesh file extensions (.fbx, .obj, .gltf, .glb) in `ImportExternalFiles()`
- [ ] After import: load mesh, run `VoxelizerAPI::VoxelizeFromFileWithColors()`
- [ ] Create entity in active scene with VoxelComponent + MeshRendererComponent (voxelized mesh)
- [ ] Add editor UI for default voxelization settings (grid size, solid/surface)
- [ ] Option to skip auto-voxelize (hold Shift on drop, or toggle in settings)

### Data Flow

```
  +----------+      +-----------+      +------------+      +----------+
  |  Source   | ---> | Voxelizer | ---> |   Voxel    | ---> | Renderer |
  |  Mesh     |      |   API     |      | Component  |      |   3D     |
  | (FBX/OBJ) |      | (existing)|      | (new)      |      | (existing)|
  +----------+      +-----------+      +------------+      +----------+
                          |
                     +----v-----+
                     | Texture  |
                     | Sampler  |
                     +----------+
```

---

## Phase 2: Physics Core

> Custom simple physics: gravity, velocity integration, AABB collision.

### RigidBodyComponent

```
File: CBEngine/src/CBEngine/Components/RigidBodyComponent.h
```

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| Velocity | `Vector3` | (0,0,0) | Linear velocity |
| AngularVelocity | `Vector3` | (0,0,0) | Angular velocity |
| Mass | `float` | 1.0 | Body mass |
| Restitution | `float` | 0.3 | Bounciness (0-1) |
| Friction | `float` | 0.5 | Surface friction (0-1) |
| UseGravity | `bool` | true | Apply gravity |
| Type | `BodyType` | Dynamic | Static / Dynamic / Kinematic |

**Tasks:**
- [ ] Create RigidBodyComponent struct with BodyType enum
- [ ] Add to `Components.h`

### ColliderComponent

```
File: CBEngine/src/CBEngine/Components/ColliderComponent.h
```

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| Type | `ColliderType` | AABB | AABB or VoxelGrid |
| HalfExtents | `Vector3` | (0.5, 0.5, 0.5) | Box half-size |
| Offset | `Vector3` | (0,0,0) | Center offset from transform |

**Tasks:**
- [ ] Create ColliderComponent struct with ColliderType enum
- [ ] Add to `Components.h`

### PhysicsSystem

```
File: CBEngine/src/CBEngine/Systems/PhysicsSystem.h/.cpp
```

**Tasks:**
- [ ] Apply gravity (`velocity.y -= 9.81 * dt`) to Dynamic bodies with UseGravity
- [ ] Integrate velocity into position (`position += velocity * dt`)
- [ ] Broad phase: collect all collider AABBs in world space
- [ ] Narrow phase: AABB-vs-AABB intersection tests
- [ ] Collision response: push apart + apply impulse (restitution)
- [ ] Wire into `Scene::OnUpdate` (currently commented out at line 91)

### Physics Pipeline

```
  PhysicsSystem::OnUpdate(dt)
  |
  +-- 1. Apply Forces
  |      For each Dynamic RigidBody:
  |        if UseGravity: velocity.y -= 9.81 * dt
  |
  +-- 2. Integrate
  |      For each Dynamic RigidBody:
  |        position += velocity * dt
  |        rotation += angularVelocity * dt
  |
  +-- 3. Detect Collisions
  |      For each pair (ColliderA, ColliderB):
  |        if AABB overlap: record collision
  |
  +-- 4. Resolve Collisions
         For each collision:
           Push apart by overlap
           Apply impulse (restitution, friction)
```

---

## Phase 3: Destruction

> Break voxelized objects on collision impact.

### DestructionSystem

```
File: CBEngine/src/CBEngine/Systems/DestructionSystem.h/.cpp
```

**Tasks:**
- [ ] Detect collision impacts above force threshold
- [ ] Remove voxels at impact point (radius-based)
- [ ] Mark VoxelComponent as dirty (triggers mesh regeneration)
- [ ] Spawn fragment entities (small voxel clusters with RigidBody)
- [ ] Track fragment lifetime, cleanup expired via `m_DeferredDestroys`

### Destruction Flow

```
  Collision Detected (PhysicsSystem)
        |
        v
  Impact Force > Threshold?
        |
     +--+--+
     |     |
    No    Yes
     |     |
   skip    +---> Remove voxels in radius
           |     Mark VoxelComponent dirty
           |
           +---> Spawn fragment entities
           |       - Small voxel clusters
           |       - Own RigidBody (Dynamic)
           |       - Lifetime timer
           |
           +---> After lifetime expires
                   Queue for deferred destroy
```

---

## Phase 4: General ECS Completeness

> Additional components for a complete engine.

### CameraComponent

```
File: CBEngine/src/CBEngine/Components/CameraComponent.h
```

- [ ] FOV, near/far planes, projection type (perspective/ortho)
- [ ] Primary camera flag for game view
- Currently camera is editor-only (`PerspectiveCameraController`)

### LightComponent

```
File: CBEngine/src/CBEngine/Components/LightComponent.h
```

- [ ] Light type: Directional / Point / Spot
- [ ] Color, intensity, range, cone angles
- [ ] Currently hardcoded in `Renderer3D::SceneData`

### LightSystem

```
File: CBEngine/src/CBEngine/Systems/LightSystem.h/.cpp
```

- [ ] Gather all LightComponent entities
- [ ] Pass to Renderer3D (replace hardcoded values)

---

## Serialization & Editor

### Serialization (`SceneSerializer.cpp`)

- [ ] CameraComponent
- [ ] LightComponent
- [ ] VoxelComponent (settings + source UUIDs, not the grid data)
- [ ] RigidBodyComponent
- [ ] ColliderComponent

### Editor Integration

- [ ] Property panel widget for each new component
- [ ] Entity picking in viewport (Framebuffer entity ID attachment)
- [ ] Light gizmos (direction arrow, range sphere)
- [ ] Collider wireframe visualization

---

## File Map

```
CBEngine/src/CBEngine/
|
+-- Components/
|   +-- Components.h              (update: add new includes)
|   +-- CoreComponents.h          [##] IDComponent, TagComponent
|   +-- TransformComponent.h      [##] Position, Rotation, Scale, Hierarchy
|   +-- MeshRendererComponent.h   [##] Mesh + Material + Shader
|   +-- VoxelComponent.h          [  ] NEW - Phase 1
|   +-- RigidBodyComponent.h      [  ] NEW - Phase 2
|   +-- ColliderComponent.h       [  ] NEW - Phase 2
|   +-- CameraComponent.h         [  ] NEW - Phase 4
|   +-- LightComponent.h          [  ] NEW - Phase 4
|
+-- Systems/
|   +-- TransformSystem.h/.cpp    [##] World matrix updates
|   +-- VoxelSystem.h/.cpp        [  ] NEW - Phase 1
|   +-- PhysicsSystem.h/.cpp      [  ] NEW - Phase 2
|   +-- DestructionSystem.h/.cpp  [  ] NEW - Phase 3
|   +-- LightSystem.h/.cpp        [  ] NEW - Phase 4
|
+-- Scene/
|   +-- Scene.cpp                 (update: wire new systems into OnUpdate)
|   +-- SceneSerializer.cpp       (update: serialize new components)
|
+-- Utils/
    +-- VoxelizerAPI.h/.cpp       [##] Reused by VoxelSystem
```
