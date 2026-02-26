# Voxel Damage Tint System + Palette Index Bug Fix

## Context
**Bug:** When voxels are destroyed and chunks/fragments spawn, mesh colors scramble. Root cause: palette indices are built in BFS/cluster-coord order but `CreatePaletteMeshFromGrid` reads them in linear grid index order (`GetFilledCoords` iterates `i = 0..totalVoxels`).

**Feature:** Modular per-damage-type tint system. Fire/Burn gradually darkens nearby voxels with spread. Impact does nothing visually. New tint types added per-substance per-damage-type in YAML. Extensible for future types (Acid, etc).

---

## Part 1: Fix Palette Index Ordering Bug

### Problem
`ExtractComponent` (VoxelSplitter.cpp:242-257) and `SpawnCollapsedChunk` (VoxelDestructionSystem.cpp:705-716) build palette indices by iterating `cluster.Coords` / `componentCoords` (BFS order). `CreatePaletteMeshFromGrid` (VoxelizerAPI.cpp:1054) consumes them via `GetFilledCoords()` which returns coords in **linear index order** (`i = 0..totalVoxels`). BFS order != linear order = wrong colors.

### Fix 1a: `CBEngine/src/CBEngine/Physics/VoxelSplitter.cpp` — ExtractComponent

Replace lines 236-257 (the `sourceFilledMap` build + `PaletteIndices.reserve` + single loop) with two passes — fill grid first, then build palette in linear order:

```cpp
// Pass 1: Fill sub-grid (order doesn't matter)
for (const auto& coord : componentCoords)
    fragment.Grid.SetFilled(coord - minC);

// Pass 2: Build palette in LINEAR order (matching GetFilledCoords/CreatePaletteMeshFromGrid)
if (!sourcePaletteIndices.empty())
{
    auto sourceFilledMap = BuildFilledIndexMap(sourceGrid);
    for (uint64_t i = 0; i < fragment.Grid.totalVoxels; ++i)
    {
        if (!fragment.Grid.IsFilled(i)) continue;
        glm::ivec3 localCoord = fragment.Grid.IndexToCoord(i);
        glm::ivec3 sourceCoord = localCoord + minC;
        uint64_t srcIdx = sourceGrid.CoordToIndex(sourceCoord);
        auto it = sourceFilledMap.find(srcIdx);
        if (it != sourceFilledMap.end() && it->second < sourcePaletteIndices.size())
            fragment.PaletteIndices.push_back(sourcePaletteIndices[it->second]);
        else
            fragment.PaletteIndices.push_back(0);
    }
}
```

**Important:** Replace lines 236-257, NOT just 242-257. The original line 237 declares `auto sourceFilledMap = BuildFilledIndexMap(sourceGrid)` and line 240 does `fragment.PaletteIndices.reserve(componentCoords.size())`. Both must be removed — the plan's Pass 2 declares its own `sourceFilledMap` and the reserve is no longer needed (palette size depends on filled voxels in fragment grid, not componentCoords count).

Note: `IndexToCoord(uint64_t)` exists in `Voxelizer.h:58`. `BuildFilledIndexMap` already exists in VoxelSplitter.

### Fix 1b: `CBEngine/src/CBEngine/Systems/VoxelDestructionSystem.cpp` — SpawnCollapsedChunk

Replace the palette extraction loop (lines 705-716). Currently it iterates `cluster.Coords` (BFS order). Fix: fill tempGrid first, then iterate tempGrid linearly. Since tempGrid has same size/origin as source, `CoordToIndex` produces the same linear index for the same coord.

```cpp
// EXISTING: Build sourceFilledMap (lines 697-703) — keep as-is

// EXISTING: Fill tempGrid from cluster.Coords (lines 736-740) — move BEFORE palette extraction

// NEW: Build palette by iterating tempGrid linearly
chunkPaletteIndices.clear();
if (!state.ModifiedPaletteIndices.empty())
{
    for (uint64_t i = 0; i < tempGrid.totalVoxels; ++i)
    {
        if (!tempGrid.IsFilled(i)) continue;
        // tempGrid has same size/origin as source, so same linear index
        auto it = sourceFilledMap.find(i);
        if (it != sourceFilledMap.end() && it->second < state.ModifiedPaletteIndices.size())
            chunkPaletteIndices.push_back(state.ModifiedPaletteIndices[it->second]);
        else
            chunkPaletteIndices.push_back(0);
    }
}
```

Reorder SpawnCollapsedChunk so tempGrid filling happens BEFORE palette extraction. The sourceFilledMap must be built from the source grid BEFORE `RemoveVoxels` is called (already the case).

---

## Part 2: Modular Damage Tint System

### 2.1 Data Structures

**`VoxelTintTypes.h`** (NEW shared header) — contains `VoxelCoordHash`, `VoxelTintEntry`, `VoxelTintMap` (see section 2.6 for full content).

**`VoxelSubstance.h`** — add after `VoxelHealthState`:

```cpp
// Per-damage-type tint configuration (loaded from YAML per substance)
struct DamageTintConfig
{
    Vector3 Color       = Vector3(1.0f);  // target tint RGB (1,1,1 = no tint)
    float   Intensity   = 0.0f;           // intensity gain per (effectiveDmg / maxHealth)
    int     SpreadRadius = 0;             // grid cells to spread, 0 = center only, max 4
    float   SpreadFalloff = 0.5f;         // intensity multiplier per cell distance
};
```

`VoxelTintEntry` lives in `VoxelTintTypes.h`, not here (shared with VoxelizerAPI).

**`VoxelSubstanceProperties`** — add one field:

```cpp
// --- Damage Tint (visual, per damage type) ---
std::unordered_map<VoxelDamageType, DamageTintConfig> DamageTints;
```

**`VoxelDamageMap.h`** — add to `EntityDestructionState`:

```cpp
using VoxelTintMap = std::unordered_map<glm::ivec3, VoxelTintEntry, VoxelCoordHash>;

// Add these fields to EntityDestructionState:
VoxelTintMap TintMap;
bool         MeshDirty = false;  // unified flag: tint change OR voxel removal needs rebuild
```

### 2.2 YAML Configuration

**`voxel_substances.yaml`** — per-substance, per-damage-type tint:

```yaml
substances:
  Wood:
    health: 60
    flammable: true
    # ... existing fields ...
    damage_tints:
      Fire:
        color: [0.12, 0.06, 0.02]      # dark charred brown
        intensity: 1.5                   # builds fast
        spread_radius: 3
        spread_falloff: 0.4
      Acid:
        color: [0.2, 0.5, 0.1]          # sickly green
        intensity: 1.0
        spread_radius: 2
        spread_falloff: 0.3

  Stone:
    health: 200
    damage_tints:
      Fire:
        color: [0.3, 0.3, 0.3]          # gray scorch
        intensity: 0.5
        spread_radius: 1
        spread_falloff: 0.3
      Explosion:
        color: [0.15, 0.1, 0.08]        # blast char
        intensity: 0.8
        spread_radius: 2
        spread_falloff: 0.5
```

Impact and other damage types without entries = no visual tint (exactly what user requested).

**`VoxelSubstanceDatabase.cpp`** — parse `damage_tints` map in `ParseEntry()`:

```cpp
static VoxelDamageType DamageTypeFromString(const std::string& s)
{
    if (s == "Impact")     return VoxelDamageType::Impact;
    if (s == "Explosion")  return VoxelDamageType::Explosion;
    if (s == "Slice")      return VoxelDamageType::Slice;
    if (s == "Fire")       return VoxelDamageType::Fire;
    if (s == "Acid")       return VoxelDamageType::Acid;
    if (s == "Pressure")   return VoxelDamageType::Pressure;
    if (s == "Structural") return VoxelDamageType::Structural;
    return VoxelDamageType::None;
}

static const char* DamageTypeToString(VoxelDamageType t)
{
    switch (t)
    {
        case VoxelDamageType::Impact:     return "Impact";
        case VoxelDamageType::Explosion:  return "Explosion";
        case VoxelDamageType::Slice:      return "Slice";
        case VoxelDamageType::Fire:       return "Fire";
        case VoxelDamageType::Acid:       return "Acid";
        case VoxelDamageType::Pressure:   return "Pressure";
        case VoxelDamageType::Structural: return "Structural";
        default:                          return "None";
    }
}

// In ParseEntry(), after existing fields:
if (node["damage_tints"] && node["damage_tints"].IsMap())
{
    for (auto it = node["damage_tints"].begin(); it != node["damage_tints"].end(); ++it)
    {
        std::string typeName = it->first.as<std::string>();
        VoxelDamageType dmgType = DamageTypeFromString(typeName);
        if (dmgType == VoxelDamageType::None) continue;

        const auto& tNode = it->second;
        DamageTintConfig cfg;
        if (tNode["color"] && tNode["color"].IsSequence() && tNode["color"].size() == 3)
            cfg.Color = Vector3(tNode["color"][0].as<float>(),
                                tNode["color"][1].as<float>(),
                                tNode["color"][2].as<float>());
        cfg.Intensity    = SafeFloat(tNode, "intensity", 0.0f);
        cfg.SpreadRadius = SafeInt(tNode, "spread_radius", 0);
        cfg.SpreadFalloff = SafeFloat(tNode, "spread_falloff", 0.5f);
        p.DamageTints[dmgType] = cfg;
    }
}
```

### 2.3 Tint Application in ProcessDamageQueue

After health update (line ~247), before the fracture stage logic, add tint logic. The `sub` variable (line 200) is already resolved:

```cpp
// Look up tint config for this damage type on this substance
auto tintCfgIt = sub.DamageTints.find(event.Type);
if (tintCfgIt != sub.DamageTints.end() && tintCfgIt->second.Intensity > 0.0f)
{
    const auto& tintCfg = tintCfgIt->second;
    float damageRatio = glm::clamp(effective / glm::max(health.MaxHealth, 0.01f), 0.0f, 1.0f);
    float addedIntensity = damageRatio * tintCfg.Intensity;

    // Tint center voxel
    auto& tint = state.TintMap[event.GridCoord];
    tint.Intensity = glm::clamp(tint.Intensity + addedIntensity, 0.0f, 1.0f);
    tint.Color = tintCfg.Color;

    // Spread to neighbors within radius
    int r = glm::min(tintCfg.SpreadRadius, 4);
    if (r > 0)
    {
        for (int dx = -r; dx <= r; ++dx)
        for (int dy = -r; dy <= r; ++dy)
        for (int dz = -r; dz <= r; ++dz)
        {
            if (dx == 0 && dy == 0 && dz == 0) continue;
            glm::ivec3 nb = event.GridCoord + glm::ivec3(dx, dy, dz);
            if (!state.ModifiedGrid.IsValidCoord(nb) || !state.ModifiedGrid.IsFilled(nb))
                continue;
            float dist = glm::length(glm::vec3(dx, dy, dz));
            if (dist > float(r)) continue;
            float spread = addedIntensity * glm::pow(tintCfg.SpreadFalloff, dist);
            if (spread < 0.01f) continue;

            auto& nTint = state.TintMap[nb];
            nTint.Color = tintCfg.Color;
            nTint.Intensity = glm::clamp(nTint.Intensity + spread, 0.0f, 1.0f);
        }
    }

    state.MeshDirty = true;
}
```

**Note on tint blending:** When a voxel receives multiple tint types (e.g. Fire then Acid), the latest write wins for Color. This is acceptable for v1 — a weighted blend system can be added later if needed.

### 2.4 Unified Mesh Rebuild — ONE rebuild per entity per frame

**OnUpdate order:**
```cpp
void VoxelDestructionSystem::OnUpdate(Scene* scene, Timestep ts)
{
    ProcessDamageQueue(scene);         // updates health, applies tints, marks MeshDirty
    ProcessPendingRemovals(scene);     // removes dead voxels, calls RebuildOrFragment, sets MeshDirty=false
    ProcessStructuralIntegrity(scene); // spawns collapsed chunks, rebuilds remaining
    ProcessDirtyMeshes(scene);         // NEW: rebuild entities with tint-only changes (no voxel deaths)
}
```

- `RebuildOrFragment` already rebuilds mesh → set `state.MeshDirty = false` after the rebuild (both single-piece and fragment paths).
- `ProcessDirtyMeshes` catches entities that had ONLY tint changes this frame (no voxel removals). Rebuilds mesh without split/fragment check.
- Use a dirty tracking vector to avoid iterating ALL entity states every frame.

**VoxelDestructionSystem.h** — add to private section:
```cpp
static void ProcessDirtyMeshes(Scene* scene);
static std::vector<uint64_t> s_TintDirtyEntities;  // entities needing tint-only mesh rebuild
```

**VoxelDestructionSystem.cpp** — add static storage + implementation:
```cpp
std::vector<uint64_t> VoxelDestructionSystem::s_TintDirtyEntities;

void VoxelDestructionSystem::ProcessDirtyMeshes(Scene* scene)
{
    if (s_TintDirtyEntities.empty()) return;

    // Deduplicate
    std::sort(s_TintDirtyEntities.begin(), s_TintDirtyEntities.end());
    s_TintDirtyEntities.erase(
        std::unique(s_TintDirtyEntities.begin(), s_TintDirtyEntities.end()),
        s_TintDirtyEntities.end());

    for (uint64_t uuid : s_TintDirtyEntities)
    {
        auto it = s_EntityStates.find(uuid);
        if (it == s_EntityStates.end()) continue;
        auto& state = it->second;
        if (!state.MeshDirty || !state.GridInitialized) continue;

        Entity entity = scene->GetEntityByUUID(UUID(uuid));
        if (!entity || !entity.HasComponent<VoxelRendererComponent>()) continue;

        auto& vr = entity.GetComponent<VoxelRendererComponent>();
        Ref<Mesh> newMesh;
        if (!state.ModifiedPaletteIndices.empty())
            newMesh = VoxelizerAPI::CreatePaletteMeshFromGrid(
                state.ModifiedGrid, state.ModifiedPaletteIndices, &state.TintMap);
        else
            newMesh = VoxelizerAPI::CreateMeshFromGrid(state.ModifiedGrid, &state.TintMap);

        if (newMesh) vr.MeshAsset = newMesh;
        state.MeshDirty = false;
    }

    s_TintDirtyEntities.clear();
}
```

In ProcessDamageQueue, when `state.MeshDirty = true` is set, also push to dirty list:
```cpp
state.MeshDirty = true;
s_TintDirtyEntities.push_back(event.EntityUUID);
```

Also clear in `Shutdown()`:
```cpp
s_TintDirtyEntities.clear();
```

**Performance note:** Tint-only mesh rebuild calls `CreatePaletteMeshFromGrid` which regenerates all vertices. For sustained damage (flamethrower), this means one mesh rebuild per entity per frame. This is acceptable — same cost as voxel removal already incurs. If profiling shows this as a bottleneck, vertex color updates could be optimized via GPU buffer sub-updates, but that requires renderer changes beyond current scope.

### 2.5 Pass Tint Data in ALL Existing Mesh Rebuilds

**RebuildOrFragment** (line ~384) — single-piece path:
```cpp
// Change existing call to pass tintMap:
newMesh = VoxelizerAPI::CreatePaletteMeshFromGrid(
    state.ModifiedGrid, state.ModifiedPaletteIndices, &state.TintMap);
// And the non-palette path:
newMesh = VoxelizerAPI::CreateMeshFromGrid(state.ModifiedGrid, &state.TintMap);
```
Add `state.MeshDirty = false;` after the `if (newMesh)` block.

**RebuildOrFragment** — fragment spawn path (line ~488):
```cpp
// Pass tint data when creating fragment mesh:
if (!frag.PaletteIndices.empty())
    fragVR.MeshAsset = VoxelizerAPI::CreatePaletteMeshFromGrid(
        frag.Grid, frag.PaletteIndices, &fragTintMap);
else
    fragVR.MeshAsset = VoxelizerAPI::CreateMeshFromGrid(frag.Grid, &fragTintMap);
```
Where `fragTintMap` is built by remapping source tint coords to fragment local space (see 2.8).

**SpawnCollapsedChunk** (line ~744):
```cpp
// Pass tint when building chunk mesh:
chunkMesh = VoxelizerAPI::CreatePaletteMeshFromGrid(tempGrid, chunkPaletteIndices, &chunkTintMap);
```
Where `chunkTintMap` is copied from source (see 2.8).

### 2.6 Mesh Generation with Tint

**New header: `CBEngine/src/CBEngine/Voxel/Destruction/VoxelTintTypes.h`** — lightweight shared header (avoids circular dependency between VoxelizerAPI and VoxelDamageMap):

```cpp
#pragma once
#include <glm/glm.hpp>
#include <unordered_map>

namespace CB
{
    struct VoxelCoordHash
    {
        size_t operator()(const glm::ivec3& c) const noexcept
        {
            size_t h = std::hash<int>()(c.x);
            h ^= std::hash<int>()(c.y) + 0x9e3779b9u + (h << 6) + (h >> 2);
            h ^= std::hash<int>()(c.z) + 0x9e3779b9u + (h << 6) + (h >> 2);
            return h;
        }
    };

    struct VoxelTintEntry
    {
        glm::vec3 Color     = glm::vec3(1.0f);
        float     Intensity = 0.0f;
    };

    using VoxelTintMap = std::unordered_map<glm::ivec3, VoxelTintEntry, VoxelCoordHash>;
}
```

Move `VoxelCoordHash` out of `VoxelDamageMap.h` into this file. `VoxelDamageMap.h` includes `VoxelTintTypes.h` instead. `VoxelTintEntry` and `VoxelTintMap` live here too. This avoids `VoxelizerAPI.h` depending on the destruction system headers.

**`VoxelizerAPI.h`** — include the new header and add optional tint parameter:

```cpp
#include "CBEngine/Voxel/Destruction/VoxelTintTypes.h"

static Ref<Mesh> CreatePaletteMeshFromGrid(const voxelizer::VoxelGrid& grid,
    const std::vector<uint8_t>& paletteIndices,
    const VoxelTintMap* tintMap = nullptr);

static Ref<Mesh> CreateMeshFromGrid(const voxelizer::VoxelGrid& grid,
    const VoxelTintMap* tintMap = nullptr);

static Ref<Mesh> CreateMeshFromGrid(const voxelizer::VoxelGrid& grid,
    const Vector3& color,
    const VoxelTintMap* tintMap = nullptr);
```

**`VoxelizerAPI.cpp`** — in `CreatePaletteMeshFromGrid` vertex loop (line ~1090), replace `v.Color = Vector3(1.0f)`:

```cpp
Vector3 tintColor(1.0f);
if (tintMap)
{
    auto it = tintMap->find(coord);
    if (it != tintMap->end())
        tintColor = glm::mix(Vector3(1.0f), it->second.Color, it->second.Intensity);
}
v.Color = tintColor;
```

**`CreateMeshFromGrid(grid, color, tintMap)`** overload (line ~1160), replace `v.Color = color`:

```cpp
Vector3 tintColor(1.0f);
if (tintMap)
{
    auto it = tintMap->find(coord);
    if (it != tintMap->end())
        tintColor = glm::mix(Vector3(1.0f), it->second.Color, it->second.Intensity);
}
v.Color = color * tintColor;  // base color × tint (NOT just tintColor!)
```

**`CreateMeshFromGrid(grid, tintMap)`** no-color overload (line ~1117) — delegates to color overload, passing tintMap through:

```cpp
Ref<Mesh> VoxelizerAPI::CreateMeshFromGrid(const voxelizer::VoxelGrid& grid, const VoxelTintMap* tintMap)
{
    return CreateMeshFromGrid(grid, Vector3(1.0f), tintMap);
}
```

**Key difference:** In `CreatePaletteMeshFromGrid`, tint goes into `v.Color` directly (palette color is handled by the shader's palette texture lookup, multiplied by `v_Color` in the shader). In `CreateMeshFromGrid`, tint is multiplied with the base `color` parameter because the vertex color IS the voxel color.

### 2.7 Shader Change

**`CBEngine-Editor/assets/shaders/PBR.glsl`** — palette mode block (line 185):

```glsl
// Before:
albedo = palColor.rgb;

// After:
albedo = palColor.rgb * v_Color;
```

Untinted voxels have `v_Color = (1,1,1)` = no visual change. Tinted voxels darken/recolor. The vertex color and use-vertex-color paths (lines 194, 201) already multiply by v_Color — only the palette path was missing this.

### 2.8 Tint Data Carried to Chunks/Fragments

**SpawnCollapsedChunk** — tempGrid uses same size/origin as source, so tint coords map directly. Build tint map for mesh creation BEFORE mesh build, and copy to chunkState after initialization (line ~830):

```cpp
// Build chunk tint map for mesh creation (BEFORE mesh build at line ~744)
VoxelTintMap chunkTintMap;
for (const auto& c : cluster.Coords)
{
    auto tintIt = state.TintMap.find(c);
    if (tintIt != state.TintMap.end())
        chunkTintMap[c] = tintIt->second;
}

// ... mesh creation uses &chunkTintMap ...

// After chunkState is initialized (line ~830):
chunkState.TintMap = std::move(chunkTintMap);
```

**RebuildOrFragment — fragment path (CRITICAL TIMING):** The source `state` is erased at line 447 (`s_EntityStates.erase(entityUUID)`) BEFORE fragment entity states are created (line ~532). The source `state.TintMap` is destroyed at that point. **Fix: save the tint map before erasing, build fragTintMap BEFORE mesh creation.**

```cpp
// BEFORE scene->DestroyEntity(entity) at line 446:
VoxelTintMap savedTintMap = std::move(state.TintMap);

// ... existing: scene->DestroyEntity + s_EntityStates.erase ...

// In the fragment spawn loop, BEFORE mesh creation at line ~487:
// Build fragTintMap by remapping source tint coords to fragment local space:
VoxelTintMap fragTintMap;
for (uint64_t fi = 0; fi < frag.Grid.totalVoxels; ++fi)
{
    if (!frag.Grid.IsFilled(fi)) continue;
    glm::ivec3 localCoord = frag.Grid.IndexToCoord(fi);
    glm::ivec3 sourceCoord = localCoord + frag.MinCoord;
    auto tintIt = savedTintMap.find(sourceCoord);
    if (tintIt != savedTintMap.end())
        fragTintMap[localCoord] = tintIt->second;
}

// Mesh creation uses &fragTintMap (line 487-491):
if (!frag.PaletteIndices.empty())
    fragVR.MeshAsset = VoxelizerAPI::CreatePaletteMeshFromGrid(
        frag.Grid, frag.PaletteIndices, &fragTintMap);
else
    fragVR.MeshAsset = VoxelizerAPI::CreateMeshFromGrid(frag.Grid, &fragTintMap);

// After fragState is created (line ~532):
fragState.TintMap = std::move(fragTintMap);
```

**Critical ordering:** `fragTintMap` MUST be built BEFORE the `CreatePaletteMeshFromGrid` call at line 487, not after. The mesh needs tint data at creation time to bake vertex colors correctly. The `frag.MinCoord` field is set by `ExtractComponent` (VoxelSplitter.h:23).

### 2.9 Tint Cleanup

When voxels are erased from the grid, also erase their tint entries. In `ProcessPendingRemovals` after `RemoveVoxels` (line ~338):

```cpp
for (const auto& coord : batch)
    state.TintMap.erase(coord);
```

In `ProcessStructuralIntegrity` where excess clusters are removed (line ~651):
```cpp
for (const auto& c : cluster.Coords)
{
    state.DamageMap.erase(c);  // existing
    state.TintMap.erase(c);    // new
}
```

In `SpawnCollapsedChunk` where cluster voxels are removed from source (line ~724):
```cpp
for (const auto& c : cluster.Coords)
{
    state.DamageMap.erase(c);  // existing
    state.TintMap.erase(c);    // new — already copied to chunkState above
}
```

### 2.10 Lua API

No new Lua bindings needed. Existing `VoxelDamage.ApplySphere` and `VoxelDamage.ApplyAtWorldPos` pass `VoxelDamageType` which drives tint lookup automatically:

```lua
-- Fire damage -> substance.DamageTints[Fire] kicks in
VoxelDamage.ApplySphere(scene, pos, 0.5, {
    type = DamageType.Fire,
    amount = 50,
})

-- Impact damage -> no tint entry for Impact -> no visual change
VoxelDamage.ApplySphere(scene, pos, 0.3, {
    type = DamageType.Impact,
    amount = 100,
})
```

---

## Part 3: Persistent Fire/Acid Spread — Worker Thread

### Context
Part 2 applies instant tint at the moment of damage. Part 3 adds **persistent spreading**: fire ignites neighbors over time, burning voxels deal tick damage, tint deepens and spreads gradually. The spread simulation runs on a **worker thread** to avoid frame stalls.

Existing infrastructure used:
- `VoxelHealthState.Burning`, `.FireTimer` — per-voxel burn state (already in `VoxelSubstance.h`)
- `VoxelSubstanceProperties.Flammable`, `.BurnDuration`, `.PropagatesDamage`, `.DamageSpreadFactor`, `.IgnitionTemperature` — substance config (already parsed from YAML)
- `VoxelizationTask` pattern (`std::future` + `std::atomic`) — existing async pattern in codebase

### 3.1 Persistent Burn State

**`VoxelDamageMap.h`** — add to `EntityDestructionState`:

```cpp
// Active burns — voxels currently on fire/dissolving
struct ActiveBurn
{
    VoxelDamageType Type = VoxelDamageType::Fire;  // Fire, Acid, etc.
    float Timer       = 0.0f;   // time remaining to burn (seconds)
    float SpreadTimer = 0.0f;   // cooldown until next spread attempt
    float TickTimer   = 0.0f;   // cooldown until next tick damage
};

using VoxelBurnMap = std::unordered_map<glm::ivec3, ActiveBurn, VoxelCoordHash>;

// Add to EntityDestructionState:
VoxelBurnMap ActiveBurns;
```

### 3.2 Ignition in ProcessDamageQueue

After tint logic in ProcessDamageQueue, when Fire/Acid damage hits a flammable substance:

```cpp
// Ignite the voxel if substance is flammable and this is Fire/Acid
if ((event.Type == VoxelDamageType::Fire || event.Type == VoxelDamageType::Acid)
    && sub.Flammable && sub.BurnDuration > 0.0f)
{
    auto& burn = state.ActiveBurns[event.GridCoord];
    if (burn.Timer <= 0.0f) // don't reset timer if already burning
    {
        burn.Type = event.Type;
        burn.Timer = sub.BurnDuration;
        burn.SpreadTimer = 0.5f;  // first spread after 0.5s
        burn.TickTimer = 0.2f;    // first tick after 0.2s
    }
    health.Burning = true;
    health.FireTimer = sub.BurnDuration;
}
```

### 3.2.1 ActiveBurns Transfer to Fragments/Chunks

When entities fragment or spawn collapsed chunks, active burns must be transferred (same pattern as TintMap in 2.8):

**RebuildOrFragment — fragment path:** Save ActiveBurns before erase, remap to fragment local space:

```cpp
// BEFORE scene->DestroyEntity at line 446 (alongside savedTintMap):
VoxelBurnMap savedBurns = std::move(state.ActiveBurns);

// In fragment spawn loop, after fragState is created (line ~532):
for (const auto& [sourceCoord, burn] : savedBurns)
{
    glm::ivec3 localCoord = sourceCoord - frag.MinCoord;
    if (frag.Grid.IsValidCoord(localCoord) && frag.Grid.IsFilled(localCoord))
        fragState.ActiveBurns[localCoord] = burn;
}
```

**SpawnCollapsedChunk:** tempGrid uses same coords as source, so no remapping needed:

```cpp
// Build chunk burn map BEFORE RemoveVoxels (alongside chunkTintMap):
VoxelBurnMap chunkBurns;
for (const auto& c : cluster.Coords)
{
    auto burnIt = state.ActiveBurns.find(c);
    if (burnIt != state.ActiveBurns.end())
        chunkBurns[c] = burnIt->second;
}

// After chunkState initialization (line ~830):
chunkState.ActiveBurns = std::move(chunkBurns);

// Also erase from source (alongside DamageMap/TintMap erase at line ~724):
state.ActiveBurns.erase(c);
```

### 3.3 Worker Thread Architecture

**New file: `CBEngine/src/CBEngine/Systems/VoxelSpreadSystem.h`**

```cpp
#pragma once
#include "CBEngine/Voxel/Destruction/VoxelDamageMap.h"
#include "CBEngine/Voxel/Destruction/VoxelSubstance.h"
#include <future>
#include <chrono>

namespace CB
{
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
        static bool s_WorkerRunning;  // tracks if a worker is in-flight
        static std::vector<SpreadResult> s_PendingResults;
        static bool s_HasPendingResults;
    };
}
```

**Note:** Removed `std::atomic` and `std::mutex` — all access to `s_WorkerFuture`, `s_WorkerRunning`, `s_PendingResults`, `s_HasPendingResults` happens on the main thread only. The `std::future` itself handles cross-thread synchronization internally.

### 3.4 Worker Thread Data Flow

**OnUpdate implementation:**
```cpp
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
        float dt = glm::min(static_cast<float>(ts), 1.0f / 30.0f); // cap to prevent spiral
        auto snapshots = BuildSnapshots(scene, dt);
        if (!snapshots.empty())
        {
            s_WorkerRunning = true;
            s_WorkerFuture = std::async(std::launch::async, ProcessSpreadTicks, std::move(snapshots));
        }
    }
}

void VoxelSpreadSystem::CollectWorkerResults()
{
    if (!s_WorkerRunning) return;
    if (s_WorkerFuture.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready)
        return; // still running — skip this frame

    s_PendingResults = s_WorkerFuture.get();
    s_HasPendingResults = true;
    s_WorkerRunning = false;
}

void VoxelSpreadSystem::Shutdown()
{
    // Wait for in-flight worker to complete before clearing state
    if (s_WorkerRunning && s_WorkerFuture.valid())
    {
        s_WorkerFuture.wait(); // blocks — but only happens on scene shutdown
        s_WorkerRunning = false;
    }
    s_PendingResults.clear();
    s_HasPendingResults = false;
}
```

**Data flow summary:**
```
Frame N:
  1. CollectWorkerResults() — non-blocking check if worker from Frame N-1 is done
     - If ready: move results to s_PendingResults, s_HasPendingResults = true
     - If not ready: skip (results applied next frame instead)

  2. ApplyPendingResults() — merge into main thread state:
     - Queue TickDamage events via VoxelDestructionSystem::QueueDamage()
     - Ignite NewIgnitions in EntityDestructionState.ActiveBurns
     - Merge TintUpdates into EntityDestructionState.TintMap
     - Remove Extinguished from ActiveBurns, clear Burning flag in DamageMap
     - Mark MeshDirty for tint changes
     - Validate all coords: skip if entity was destroyed or coord no longer filled

  3. BuildSnapshots() + Launch worker (only if !s_WorkerRunning):
     - Deep copy Grid + ActiveBurns + Substance into SpreadSnapshot per entity
     - s_WorkerFuture = std::async(std::launch::async, ProcessSpreadTicks, snapshots)
```

### 3.5 Worker Thread — ProcessSingleEntity (Pure Function)

```cpp
SpreadResult VoxelSpreadSystem::ProcessSingleEntity(const SpreadSnapshot& snap)
{
    SpreadResult result;
    result.EntityUUID = snap.EntityUUID;
    result.UpdatedBurns = snap.Burns; // start from current state

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
            tickEvt.RawAmount = snap.Substance.Health * 0.05f; // 5% max HP per tick
            result.TickDamage.push_back(tickEvt);

            // Deepen tint on burning voxel
            auto& tint = result.TintUpdates[coord];
            auto tintCfgIt = snap.Substance.DamageTints.find(burn.Type);
            if (tintCfgIt != snap.Substance.DamageTints.end())
            {
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

                // 6-connected neighbors
                static const glm::ivec3 neighbors[6] = {
                    {1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}
                };
                for (const auto& offset : neighbors)
                {
                    glm::ivec3 nb = coord + offset;
                    if (!snap.Grid.IsValidCoord(nb) || !snap.Grid.IsFilled(nb))
                        continue;
                    // Don't re-ignite already burning
                    if (result.UpdatedBurns.count(nb)) continue;

                    // Spread with probability based on DamageSpreadFactor
                    // Deterministic: use coord hash as pseudo-random seed
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
```

### 3.6 System Registration

Register `VoxelSpreadSystem` via `VoxelSpreadSystemAdapter` in `Scene::InitPhysics()` at priority **176** (after VoxelDestructionSystem at 175, so results are applied before next frame's damage processing).

```cpp
class VoxelSpreadSystemAdapter : public ISystem
{
public:
    void OnUpdate(Scene* scene, Timestep ts) override { VoxelSpreadSystem::OnUpdate(scene, ts); }
    void Shutdown() override { VoxelSpreadSystem::Shutdown(); }
    const char* GetName() const override { return "VoxelSpreadSystem"; }
    int GetPriority() const override { return 176; }
};
```

### 3.7 Thread Safety Guarantees

- **Worker thread reads only from snapshot copies** — no shared mutable state. `SpreadSnapshot` is a deep copy (grid data, burn map, substance props). Pure function.
- **Results applied on main thread only** — `ApplyPendingResults` runs at start of `OnUpdate`, before any other mutation.
- **No mutex needed** — all `s_WorkerRunning`, `s_PendingResults`, `s_HasPendingResults` access happens on main thread only. `std::future::get()` handles cross-thread synchronization internally.
- **Worker launch guard** — `OnUpdate` only launches a new worker when `!s_WorkerRunning`. `CollectWorkerResults` sets `s_WorkerRunning = false` only after `future.get()` completes. Prevents overlapping workers.
- **Shutdown safety** — `Shutdown()` blocks on `s_WorkerFuture.wait()` to ensure worker completes before scene data is destroyed.
- **1-frame latency** — spread results are visible one frame after computation. Acceptable for gradual fire spread (imperceptible to player).
- **Grid staleness** — worker may process a voxel that was destroyed on the main thread since the snapshot. Main thread validates results before applying (skip invalid coords, skip non-filled voxels, skip erased entities).

### 3.8 YAML — No New Fields Needed

Existing substance fields already support spread:
```yaml
Wood:
  flammable: true
  burn_duration: 5.0          # burns for 5 seconds
  propagates_damage: true     # fire spreads to neighbors
  damage_spread_factor: 0.5   # 50% chance per tick per neighbor
  ignition_temperature: 0.0   # ignites immediately on fire damage
```

### 3.9 Files for Part 3

| File | Changes |
|------|---------|
| **NEW** `CBEngine/src/CBEngine/Systems/VoxelSpreadSystem.h` | System class, snapshot/result structs |
| **NEW** `CBEngine/src/CBEngine/Systems/VoxelSpreadSystem.cpp` | Worker thread logic, apply results, snapshot building |
| `CBEngine/src/CBEngine/Voxel/Destruction/VoxelDamageMap.h` | Add `ActiveBurn` struct, `VoxelBurnMap`, `ActiveBurns` field |
| `CBEngine/src/CBEngine/Systems/VoxelDestructionSystem.cpp` | Add ignition logic in ProcessDamageQueue |
| `CBEngine/src/CBEngine/Scene/Scene.cpp` | Register `VoxelSpreadSystemAdapter` at priority 176 |

---

## Part 4: Substance Config Editor Panel

### Context
Currently, tweaking substance properties requires editing `voxel_substances.yaml` manually and restarting. An ImGui editor panel allows live tuning of all substance properties (including the new damage tint configs) with instant feedback.

### 4.1 VoxelSubstanceDatabase — Add Mutable Access + Save

**`VoxelSubstanceDatabase.h`** — add two new public methods:

```cpp
// Mutable access for editor panel (returns reference to internal storage)
static VoxelSubstanceProperties& GetMutable(VoxelMaterialType type);

// Serialize current state back to YAML file
static void Save(const std::string& path);
```

**`VoxelSubstanceDatabase.cpp`** — implement:

```cpp
VoxelSubstanceProperties& VoxelSubstanceDatabase::GetMutable(VoxelMaterialType type)
{
    int idx = static_cast<int>(type);
    if (idx < 0 || idx >= static_cast<int>(VoxelMaterialType::Count))
        return s_Fallback;
    return s_Properties[idx];
}

void VoxelSubstanceDatabase::Save(const std::string& path)
{
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "substances" << YAML::Value << YAML::BeginMap;

    for (int i = 0; i < static_cast<int>(VoxelMaterialType::Count); ++i)
    {
        VoxelMaterialType type = static_cast<VoxelMaterialType>(i);
        const auto& p = s_Properties[i];
        out << YAML::Key << VoxelMaterialTypeToString(type);
        out << YAML::Value << YAML::BeginMap;

        out << YAML::Key << "mass_per_voxel"        << YAML::Value << p.MassPerVoxel;
        out << YAML::Key << "health"                 << YAML::Value << p.Health;
        out << YAML::Key << "hardness"               << YAML::Value << p.Hardness;
        out << YAML::Key << "explosion_resistance"   << YAML::Value << p.ExplosionResistance;
        out << YAML::Key << "slice_resistance"       << YAML::Value << p.SliceResistance;
        out << YAML::Key << "tensile_strength"       << YAML::Value << p.TensileStrength;
        out << YAML::Key << "impact_threshold"       << YAML::Value << p.ImpactThreshold;

        // Fracture
        const char* fracNames[] = { "NONE", "CHIP", "CRACK", "SHATTER", "CRUMBLE" };
        out << YAML::Key << "fracture_behavior"      << YAML::Value << fracNames[static_cast<int>(p.Fracture)];
        out << YAML::Key << "fracture_threshold"     << YAML::Value << p.FractureThreshold;
        out << YAML::Key << "fragment_count"         << YAML::Value << p.FragmentCount;
        out << YAML::Key << "fragments_have_physics" << YAML::Value << p.FragmentsHavePhysics;

        // Environmental
        out << YAML::Key << "flammable"              << YAML::Value << p.Flammable;
        out << YAML::Key << "ignition_temperature"   << YAML::Value << p.IgnitionTemperature;
        out << YAML::Key << "burn_duration"          << YAML::Value << p.BurnDuration;
        out << YAML::Key << "propagates_damage"      << YAML::Value << p.PropagatesDamage;
        out << YAML::Key << "damage_spread_factor"   << YAML::Value << p.DamageSpreadFactor;

        // Damage tints
        if (!p.DamageTints.empty())
        {
            out << YAML::Key << "damage_tints" << YAML::Value << YAML::BeginMap;
            for (const auto& [dmgType, cfg] : p.DamageTints)
            {
                out << YAML::Key << DamageTypeToString(dmgType);
                out << YAML::Value << YAML::BeginMap;
                out << YAML::Key << "color" << YAML::Value << YAML::Flow
                    << YAML::BeginSeq << cfg.Color.x << cfg.Color.y << cfg.Color.z << YAML::EndSeq;
                out << YAML::Key << "intensity"      << YAML::Value << cfg.Intensity;
                out << YAML::Key << "spread_radius"  << YAML::Value << cfg.SpreadRadius;
                out << YAML::Key << "spread_falloff" << YAML::Value << cfg.SpreadFalloff;
                out << YAML::EndMap;
            }
            out << YAML::EndMap;
        }

        out << YAML::EndMap;
    }

    out << YAML::EndMap << YAML::EndMap;

    std::ofstream file(path);
    file << out.c_str();
}
```

### 4.2 New Panel: `SubstanceEditorPanel`

**New file: `CBEngine-Editor/src/Panels/SubstanceEditorPanel.h`**

Follows existing Panel base class pattern. Substance selected via tab bar or combo, all properties shown with appropriate ImGui controls.

```cpp
#pragma once
#include "Panel.h"
#include "CBEngine/Utils/VoxelMaterialType.h"

namespace CB
{
    class SubstanceEditorPanel : public Panel
    {
    public:
        SubstanceEditorPanel()
            : Panel("Substance Editor", false) {} // hidden by default

        void OnImGuiRender() override;

    private:
        void DrawSubstance(VoxelMaterialType type);
        void DrawDamageTints(VoxelSubstanceProperties& props);

        int m_SelectedSubstance = 0;  // index into VoxelMaterialType
        bool m_AutoReload = true;     // auto-apply changes to runtime
    };
}
```

### 4.3 Panel Layout

**`CBEngine-Editor/src/Panels/SubstanceEditorPanel.cpp`**

```cpp
void SubstanceEditorPanel::OnImGuiRender()
{
    if (!m_Visible) return;

    ImGui::Begin("Substance Editor", &m_Visible);

    // Substance selector (tab bar with one tab per material type)
    if (ImGui::BeginTabBar("Substances"))
    {
        for (int i = 0; i < static_cast<int>(VoxelMaterialType::Count); ++i)
        {
            VoxelMaterialType type = static_cast<VoxelMaterialType>(i);
            const char* name = VoxelMaterialTypeToString(type);
            if (ImGui::BeginTabItem(name))
            {
                m_SelectedSubstance = i;
                DrawSubstance(type);
                ImGui::EndTabItem();
            }
        }
        ImGui::EndTabBar();
    }

    ImGui::Separator();

    // Save/Reload buttons
    if (ImGui::Button("Save to YAML"))
        VoxelSubstanceDatabase::Save("assets/config/voxel_substances.yaml");
    ImGui::SameLine();
    if (ImGui::Button("Reload from YAML"))
        VoxelSubstanceDatabase::Load("assets/config/voxel_substances.yaml");

    ImGui::End();
}

void SubstanceEditorPanel::DrawSubstance(VoxelMaterialType type)
{
    auto& p = VoxelSubstanceDatabase::GetMutable(type);

    // --- Physical ---
    if (ImGui::CollapsingHeader("Physical", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::DragFloat("Mass Per Voxel", &p.MassPerVoxel, 0.001f, 0.001f, 10.0f, "%.4f");
    }

    // --- Structural ---
    if (ImGui::CollapsingHeader("Structural", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::DragFloat("Health",              &p.Health,              1.0f, 1.0f, 10000.0f);
        ImGui::SliderFloat("Hardness",          &p.Hardness,           0.0f, 100.0f);
        ImGui::SliderFloat("Explosion Resist",  &p.ExplosionResistance, 0.0f, 1.0f);
        ImGui::SliderFloat("Slice Resist",      &p.SliceResistance,    0.0f, 1.0f);
        ImGui::DragFloat("Tensile Strength",    &p.TensileStrength,    1.0f, 0.0f, 100000.0f);
        ImGui::DragFloat("Impact Threshold",    &p.ImpactThreshold,    0.1f, 0.0f, 1000.0f);
    }

    // --- Fracture ---
    if (ImGui::CollapsingHeader("Fracture"))
    {
        const char* fractureNames[] = { "None", "Chip", "Crack", "Shatter", "Crumble" };
        int fracIdx = static_cast<int>(p.Fracture);
        if (ImGui::Combo("Behavior", &fracIdx, fractureNames, 5))
            p.Fracture = static_cast<FractureBehavior>(fracIdx);
        ImGui::SliderFloat("Fracture Threshold", &p.FractureThreshold, 0.0f, 1.0f);
        ImGui::SliderInt("Fragment Count",        &p.FragmentCount, 0, 20);
        ImGui::Checkbox("Fragments Have Physics",  &p.FragmentsHavePhysics);
    }

    // --- Environmental ---
    if (ImGui::CollapsingHeader("Environmental"))
    {
        ImGui::Checkbox("Flammable", &p.Flammable);
        if (p.Flammable)
        {
            ImGui::DragFloat("Ignition Temperature", &p.IgnitionTemperature, 1.0f, 0.0f, 1000.0f);
            ImGui::DragFloat("Burn Duration (s)",     &p.BurnDuration,        0.1f, 0.0f, 60.0f);
        }
        ImGui::Checkbox("Propagates Damage",  &p.PropagatesDamage);
        if (p.PropagatesDamage)
            ImGui::SliderFloat("Spread Factor", &p.DamageSpreadFactor, 0.0f, 1.0f);
    }

    // --- Damage Tints ---
    DrawDamageTints(p);
}

void SubstanceEditorPanel::DrawDamageTints(VoxelSubstanceProperties& props)
{
    if (!ImGui::CollapsingHeader("Damage Tints"))
        return;

    // Show existing tint configs
    static const char* damageTypeNames[] = {
        "Impact", "Explosion", "Slice", "Fire", "Acid", "Pressure", "Structural"
    };
    static const VoxelDamageType damageTypeValues[] = {
        VoxelDamageType::Impact, VoxelDamageType::Explosion, VoxelDamageType::Slice,
        VoxelDamageType::Fire, VoxelDamageType::Acid, VoxelDamageType::Pressure,
        VoxelDamageType::Structural
    };

    // Iterate configured tints
    std::vector<VoxelDamageType> toRemove;
    for (auto& [dmgType, cfg] : props.DamageTints)
    {
        // Find name for this type
        const char* typeName = "Unknown";
        for (int i = 0; i < 7; ++i)
            if (damageTypeValues[i] == dmgType) { typeName = damageTypeNames[i]; break; }

        ImGui::PushID(static_cast<int>(dmgType));
        if (ImGui::TreeNode(typeName))
        {
            float col[3] = { cfg.Color.x, cfg.Color.y, cfg.Color.z };
            if (ImGui::ColorEdit3("Tint Color", col))
                cfg.Color = Vector3(col[0], col[1], col[2]);
            ImGui::DragFloat("Intensity",     &cfg.Intensity,    0.01f, 0.0f, 5.0f);
            ImGui::SliderInt("Spread Radius",  &cfg.SpreadRadius, 0, 4);
            ImGui::SliderFloat("Spread Falloff", &cfg.SpreadFalloff, 0.0f, 1.0f);

            if (ImGui::Button("Remove"))
                toRemove.push_back(dmgType);

            ImGui::TreePop();
        }
        ImGui::PopID();
    }

    for (auto t : toRemove)
        props.DamageTints.erase(t);

    // Add new tint config
    ImGui::Separator();
    static int addTypeIdx = 3; // default to Fire
    ImGui::Combo("##AddType", &addTypeIdx, damageTypeNames, 7);
    ImGui::SameLine();
    if (ImGui::Button("Add Tint"))
    {
        VoxelDamageType addType = damageTypeValues[addTypeIdx];
        if (props.DamageTints.find(addType) == props.DamageTints.end())
            props.DamageTints[addType] = DamageTintConfig{};
    }
}
```

### 4.4 Registration in EditorLayer

**`CBEngine-Editor/src/EditorLayer.h`** — add member:
```cpp
#include "Panels/SubstanceEditorPanel.h"
SubstanceEditorPanel m_SubstanceEditorPanel;
```

**`CBEngine-Editor/src/EditorLayer.cpp`** — in `OnImGuiRender()` add:
```cpp
m_SubstanceEditorPanel.OnImGuiRender();
```

In the Tools menu (lines ~342-348):
```cpp
ImGui::MenuItem("Substance Editor", nullptr, m_SubstanceEditorPanel.GetVisiblePtr());
```

### 4.5 Files for Part 4

| File | Changes |
|------|---------|
| **NEW** `CBEngine-Editor/src/Panels/SubstanceEditorPanel.h` | Panel class definition |
| **NEW** `CBEngine-Editor/src/Panels/SubstanceEditorPanel.cpp` | ImGui rendering, all controls |
| `CBEngine/src/CBEngine/Voxel/Destruction/VoxelSubstanceDatabase.h` | Add `GetMutable()`, `Save()` |
| `CBEngine/src/CBEngine/Voxel/Destruction/VoxelSubstanceDatabase.cpp` | Implement `GetMutable()`, `Save()` (YAML emit) |
| `CBEngine-Editor/src/EditorLayer.h` | Add `SubstanceEditorPanel` member |
| `CBEngine-Editor/src/EditorLayer.cpp` | Render panel + add Tools menu item |

---

## Performance Considerations

1. **Instant tint spread is O((2r+1)^3) per damage event.** With max r=4: 729 iterations. A 50-voxel explosion = ~36K neighbor checks — negligible vs the physics/mesh rebuild cost.

2. **Tint-only mesh rebuild** calls `CreatePaletteMeshFromGrid` (full vertex regeneration). One rebuild per entity per frame max — same cost as voxel removal already incurs.

3. **Dirty entity tracking** via `s_TintDirtyEntities` vector avoids iterating all entity states in `ProcessDirtyMeshes`.

4. **Worker thread snapshot cost:** Deep-copying `VoxelGrid` for each entity with active burns. Grid data is bit-packed (`totalVoxels/64` uint64_t values). For a 64x64x64 grid = 4096 uint64_t = 32KB. With ~5 burning entities = 160KB snapshot per frame. Acceptable.

5. **Worker thread spread cost:** O(active_burns * 6_neighbors) per entity per tick. With 500 burning voxels and 6 neighbors = 3000 checks. Trivial workload — worker thread will finish well within one frame.

6. **Grid staleness mitigation:** Worker operates on stale grid snapshot (1-frame old). Main thread validates all results before applying — skips coords that are no longer valid/filled. Worst case: a spread event targets a just-destroyed voxel and gets silently dropped.

7. **Memory:** VoxelTintEntry = 16 bytes/entry. ActiveBurn = ~16 bytes/entry. 10K tint entries + 500 burns = ~200KB per entity. Well within budget.

8. **`VoxelSubstanceProperties` now contains `std::unordered_map`**, making it non-trivially copyable. Stored in a static array (~5 entries), loaded once at startup. Copied once per burning entity per frame in `BuildSnapshots` — each substance has ~1-3 tint configs, so the map copy cost is negligible (< 100 bytes per entity).

---

## Files Modified (All Parts)

| File | Part | Changes |
|------|------|---------|
| **NEW** `CBEngine/src/CBEngine/Voxel/Destruction/VoxelTintTypes.h` | 2 | `VoxelCoordHash`, `VoxelTintEntry`, `VoxelTintMap` |
| `CBEngine/src/CBEngine/Voxel/Destruction/VoxelSubstance.h` | 2 | Add `DamageTintConfig`, `DamageTints` map |
| `CBEngine/src/CBEngine/Voxel/Destruction/VoxelDamageMap.h` | 2,3 | `TintMap`, `MeshDirty`, `ActiveBurn`, `VoxelBurnMap`, `ActiveBurns` |
| `CBEngine/src/CBEngine/Voxel/Destruction/VoxelSubstanceDatabase.h` | 2,4 | `GetMutable()`, `Save()` |
| `CBEngine/src/CBEngine/Voxel/Destruction/VoxelSubstanceDatabase.cpp` | 2,4 | Parse `damage_tints`, `DamageTypeFromString`, `GetMutable()`, `Save()` |
| `CBEngine/src/CBEngine/Systems/VoxelDestructionSystem.h` | 2 | `ProcessDirtyMeshes`, `s_TintDirtyEntities` |
| `CBEngine/src/CBEngine/Systems/VoxelDestructionSystem.cpp` | 1,2,3 | Palette fix, tint logic, ProcessDirtyMeshes, ignition, tint copy/cleanup |
| `CBEngine/src/CBEngine/Physics/VoxelSplitter.cpp` | 1 | Fix ExtractComponent palette ordering (two-pass) |
| `CBEngine/src/CBEngine/Utils/VoxelizerAPI.h` | 2 | `VoxelTintMap*` param on mesh creation |
| `CBEngine/src/CBEngine/Utils/VoxelizerAPI.cpp` | 2 | Apply tint to vertex colors |
| `CBEngine-Editor/assets/shaders/PBR.glsl` | 2 | `albedo = palColor.rgb * v_Color` |
| `CBEngine-Editor/assets/config/voxel_substances.yaml` | 2 | `damage_tints` per substance |
| **NEW** `CBEngine/src/CBEngine/Systems/VoxelSpreadSystem.h` | 3 | Worker thread system |
| **NEW** `CBEngine/src/CBEngine/Systems/VoxelSpreadSystem.cpp` | 3 | Spread logic, snapshots, results |
| `CBEngine/src/CBEngine/Scene/Scene.cpp` | 3 | Register `VoxelSpreadSystemAdapter` |
| **NEW** `CBEngine-Editor/src/Panels/SubstanceEditorPanel.h` | 4 | Editor panel class |
| **NEW** `CBEngine-Editor/src/Panels/SubstanceEditorPanel.cpp` | 4 | ImGui controls for all substance props |
| `CBEngine-Editor/src/EditorLayer.h` | 4 | Add panel member |
| `CBEngine-Editor/src/EditorLayer.cpp` | 4 | Render panel + Tools menu item |

---

## Implementation Order

**Phase A: Palette Fix (Part 1)** — independent, verify immediately
1. Fix ExtractComponent two-pass palette ordering
2. Fix SpawnCollapsedChunk palette ordering

**Phase B: Instant Tint System (Part 2)** — verify after each step
3. Create `VoxelTintTypes.h`, update `VoxelDamageMap.h`
4. Add `DamageTintConfig` to `VoxelSubstance.h`, `DamageTints` to `VoxelSubstanceProperties`
5. Parse `damage_tints` in `VoxelSubstanceDatabase.cpp`
6. Add tint param to `VoxelizerAPI.h/.cpp`, shader change in `PBR.glsl`
7. Tint logic in ProcessDamageQueue + ProcessDirtyMeshes
8. Tint propagation to chunks/fragments + cleanup
9. Update `voxel_substances.yaml` with Fire/Acid tint configs

**Phase C: Persistent Fire Spread (Part 3)** — verify after complete
10. Add `ActiveBurn` / `VoxelBurnMap` to `VoxelDamageMap.h`
11. Add ignition logic to ProcessDamageQueue
12. Create `VoxelSpreadSystem.h/.cpp` with worker thread
13. Register system adapter in Scene
14. End-to-end fire spread test

**Phase D: Substance Editor Panel (Part 4)** — verify after complete
15. Add `GetMutable()` + `Save()` to `VoxelSubstanceDatabase`
16. Create `SubstanceEditorPanel.h/.cpp`
17. Register in EditorLayer (member + Tools menu)
18. Test: tweak values, save to YAML, reload

## Verification

**Part 1 — Palette fix:**
1. Destroy voxels causing fragments/chunks -> colors stay correct on all pieces

**Part 2 — Instant tint:**
2. Apply `DamageType.Fire` -> nearby voxels darken with charred color
3. Continue applying Fire -> darkening spreads further and deepens
4. Apply `DamageType.Impact` -> voxels take damage but no visual tint
5. Add temporary log: each entity rebuilds at most once per frame
6. Destroy tinted region -> spawned fragments/chunks retain char colors
7. Apply Fire then Acid to same area -> latest tint color wins, intensity stacks
8. Destroyed voxels don't leak tint entries (check `TintMap.size()` via debug)

**Part 3 — Persistent fire spread:**
9. Set Wood flammable=true, burn_duration=5, propagates_damage=true in YAML
10. Apply Fire damage to Wood -> fire ignites, continues burning for 5 seconds
11. Burning voxels deal tick damage (health decreases over time without new input)
12. Fire spreads to adjacent flammable voxels after ~0.5s delay
13. Fire spread deepens tint on burning voxels automatically
14. Fire extinguishes after burn_duration expires
15. Worker thread doesn't cause crashes or data races (stress test: multiple entities burning simultaneously)

**Part 4 — Substance Editor Panel:**
16. Open panel from Tools -> Substance Editor
17. Tab through substances (Stone, Wood, Metal, Glass, Marble) — all fields visible
18. Edit Health/Hardness values -> changes take effect immediately on next damage
19. Add/remove damage tint configs (Fire tint on Wood) -> tint appears on fire damage
20. Color picker for tint color -> visual preview matches
21. Save to YAML -> close and reopen editor -> values persist
22. Reload from YAML -> reverts any unsaved changes

---

## Review Changelog — Bugs Found & Fixed

### Review Round 1: Performance & Architecture

**Bug R1-1: ProcessDirtyMeshes iterates ALL entity states every frame**
- **Where:** Part 2, Section 2.4 — `ProcessDirtyMeshes` implementation
- **Problem:** Original design iterated `s_EntityStates` (potentially hundreds of entries) every frame to find entities with `MeshDirty=true`. Wasteful when only 1-2 entities have tint changes.
- **Fix:** Added `s_TintDirtyEntities` tracking vector. ProcessDamageQueue pushes entity UUIDs when setting `MeshDirty=true`. ProcessDirtyMeshes only processes entities in this vector (with sort+unique dedup). O(dirty) instead of O(all).

**Bug R1-2: Fragment tint copy timing — source state erased before fragments access TintMap**
- **Where:** Part 2, Section 2.8 — `RebuildOrFragment` fragment path
- **Problem:** `s_EntityStates.erase(entityUUID)` at line 447 destroys `state.TintMap` BEFORE fragment entity states are created at line ~532. Fragment tint data would be lost.
- **Fix:** Save tint map before erase: `VoxelTintMap savedTintMap = std::move(state.TintMap)` before `scene->DestroyEntity()`. Use `savedTintMap` when populating fragment tint data.

**Bug R1-3: VoxelTintMap forward declaration won't compile**
- **Where:** Part 2, Section 2.6 — `VoxelizerAPI.h` header
- **Problem:** Original plan used `using VoxelTintMap = ...` as a forward declaration in VoxelizerAPI.h. `using` aliases cannot be forward-declared in C++.
- **Fix:** Created `VoxelTintTypes.h` shared header containing `VoxelCoordHash`, `VoxelTintEntry`, and `VoxelTintMap`. VoxelizerAPI.h includes this header directly. Moved `VoxelCoordHash` out of `VoxelDamageMap.h` into VoxelTintTypes.h (VoxelDamageMap.h includes it transitively). Avoids circular dependency between VoxelizerAPI and VoxelDamageMap.

### Review Round 2: Worker Thread & Save()

**Bug R2-1: Worker thread launch guard missing**
- **Where:** Part 3, Section 3.4 — `VoxelSpreadSystem::OnUpdate`
- **Problem:** `OnUpdate` launched `std::async` every frame without checking if the previous worker was still running. Could cause overlapping workers (undefined behavior with `std::future` reassignment while previous is still executing) or blocking on future destruction.
- **Fix:** Added `s_WorkerRunning` bool flag. `CollectWorkerResults()` non-blocking checks `s_WorkerFuture.wait_for(0ms)` and only sets `s_WorkerRunning=false` after `future.get()` completes. `OnUpdate` only launches when `!s_WorkerRunning`. Full code provided in section 3.4.

**Bug R2-2: Shutdown() implementation missing — worker future blocks on destruction**
- **Where:** Part 3, Section 3.4 — `VoxelSpreadSystem::Shutdown`
- **Problem:** No `Shutdown()` implementation was specified. `std::future` destructor blocks until the worker completes, but by that time scene data may already be destroyed, leading to use-after-free if worker results reference stale pointers.
- **Fix:** Explicit `Shutdown()` that calls `s_WorkerFuture.wait()` (blocking, but only on scene shutdown), then clears all state: `s_WorkerRunning=false`, `s_PendingResults.clear()`, `s_HasPendingResults=false`. Full code in section 3.4.

**Bug R2-3: Save() has placeholder comments instead of actual serialization**
- **Where:** Part 4, Section 4.1 — `VoxelSubstanceDatabase::Save()`
- **Problem:** Lines for fracture behavior, fracture threshold, fragment count, fragments_have_physics, flammable, ignition_temperature, burn_duration, propagates_damage, damage_spread_factor, and the entire damage_tints map were written as `// ...` placeholder comments. Would silently lose all fracture/environmental/tint data on save.
- **Fix:** Full YAML emission code for ALL fields: fracture behavior (string via `fracNames[]` array), all numeric/bool fields, and damage_tints map with nested per-type emission (color as Flow sequence, intensity, spread_radius, spread_falloff). Also added `DamageTypeToString()` helper needed by Save().

**Bug R2-4: ActiveBurns not transferred to fragments/chunks**
- **Where:** Part 3, Section 3.2.1 (new section added)
- **Problem:** When `RebuildOrFragment` fragments an entity (line 446: DestroyEntity + erase), the `ActiveBurns` map was destroyed along with the entity state. Fire would stop on fragmentation. Same issue with `SpawnCollapsedChunk` — active burns on cluster voxels were not transferred to the chunk entity.
- **Fix:** Added section 3.2.1 with explicit transfer logic:
  - **Fragment path:** `VoxelBurnMap savedBurns = std::move(state.ActiveBurns)` before erase. In fragment loop: iterate `savedBurns`, remap `sourceCoord - frag.MinCoord` to fragment local space, copy matching burns to `fragState.ActiveBurns`.
  - **Chunk path:** Build `chunkBurns` from `cluster.Coords` BEFORE `RemoveVoxels`. Move to `chunkState.ActiveBurns` after initialization. Erase transferred burns from source alongside DamageMap/TintMap erase.

**Bug R2-5: Unnecessary std::atomic/std::mutex in VoxelSpreadSystem**
- **Where:** Part 3, Section 3.3 — `VoxelSpreadSystem.h` header
- **Problem:** Original design included `std::atomic<bool> s_HasPendingResults` and `std::mutex s_ResultMutex`. All access to these fields (collect results, apply results, launch worker, shutdown) happens exclusively on the main thread. The mutex/atomic overhead is unnecessary.
- **Fix:** Replaced `std::atomic<bool>` with plain `bool`, removed `std::mutex`. `std::future` itself handles cross-thread synchronization for the result handoff. Updated includes to remove `<atomic>` and `<mutex>`, added `<chrono>` (needed for `wait_for(0ms)`).

### Review Round 3: Compile Errors & Data Correctness

**Bug R3-1: Duplicate `sourceFilledMap` variable declaration (compile error)**
- **Where:** Part 1, Section Fix 1a — `ExtractComponent` in VoxelSplitter.cpp
- **Problem:** Original plan said "Replace lines 242-257" but the existing line 237 declares `auto sourceFilledMap = BuildFilledIndexMap(sourceGrid)`. The plan's Pass 2 code also declares `auto sourceFilledMap = BuildFilledIndexMap(sourceGrid)`. Since line 237 is outside the 242-257 replacement range, both declarations would coexist in the same scope → C++ compile error (variable redefinition).
- **Fix:** Changed replacement range to lines 236-257 (includes the original `sourceFilledMap` declaration at 237 and `PaletteIndices.reserve` at 240). The plan's Pass 2 now owns the only `sourceFilledMap` declaration. Added explicit warning note in the plan.

**Bug R3-2: Fragment tint map built AFTER mesh creation — tint not applied**
- **Where:** Part 2, Section 2.8 — `RebuildOrFragment` fragment path
- **Problem:** Plan said "In the fragment spawn loop, after fragState is created (line ~532): remap source tint coords". But the mesh is created at line ~487, BEFORE fragState at line ~532. The `fragTintMap` passed to `CreatePaletteMeshFromGrid` would not exist yet → either compile error (undefined variable) or mesh created without tint data.
- **Fix:** Reordered to build `fragTintMap` BEFORE the mesh creation call. New ordering in the fragment loop:
  1. Build `fragTintMap` from `savedTintMap` (remap sourceCoord → localCoord)
  2. Create mesh with `&fragTintMap`
  3. After `fragState` is created: `fragState.TintMap = std::move(fragTintMap)`
  Added explicit "Critical ordering" note in section 2.8.

**Bug R3-3: Color overload tint overwrites base color instead of multiplying**
- **Where:** Part 2, Section 2.6 — `VoxelizerAPI.cpp` tint application
- **Problem:** Plan said "Same change in CreateMeshFromGrid overloads where v.Color is set" — implying `v.Color = tintColor` for all overloads. But in `CreateMeshFromGrid(grid, color)`, the `color` parameter IS the base voxel color. Setting `v.Color = tintColor` would discard it, replacing all voxel colors with the tint color.
- **Fix:** Explicit per-overload code:
  - `CreatePaletteMeshFromGrid`: `v.Color = tintColor` (palette color is separate, from texture)
  - `CreateMeshFromGrid(grid, color, tintMap)`: `v.Color = color * tintColor` (multiply base × tint)
  - `CreateMeshFromGrid(grid, tintMap)`: delegates to color overload with `Vector3(1.0f)` + tintMap passthrough
  Added "Key difference" explanation at the end of section 2.6.

### Verified (No Bugs)

| Item | Why it's correct |
|------|-----------------|
| `Vector3` vs `glm::vec3` type mismatch | `CoreMath.h` defines `using Vector3 = glm::vec3` — same type |
| `VoxelCoordHash` transitive include | VoxelTintTypes.h → VoxelDamageMap.h → VoxelStructuralIntegrity.h — chain intact |
| `std::hash<int>` without `<functional>` | `<unordered_map>` (included in VoxelTintTypes.h) pulls in `<functional>` on all major compilers |
| Tint cleanup timing in ProcessPendingRemovals | Dead voxel tints erased AFTER RemoveVoxels but BEFORE RebuildOrFragment. Surviving tints preserved for fragments. |
| ProcessDirtyMeshes double-rebuild | RebuildOrFragment sets `MeshDirty=false`, so ProcessDirtyMeshes skips already-rebuilt entities |
| Worker TintUpdates without s_TintDirtyEntities push | Worker always pairs TintUpdates with TickDamage; TickDamage goes through ProcessDamageQueue which sets MeshDirty + pushes to s_TintDirtyEntities |
| `VoxelDamageType` bitfield enum as unordered_map key | `uint32_t` underlying type — `std::hash<uint32_t>` used automatically |
| SpawnCollapsedChunk tempGrid same dimensions | Line 729: `tempGrid.size = state.ModifiedGrid.size` — confirmed same dimensions, linear index maps correctly |
| Fragment `MinCoord` availability | `VoxelFragment::MinCoord` set at VoxelSplitter.cpp:221, available for tint remapping |
| PBR.glsl `v_Color` availability | Declared as fragment input at line 69, set from `a_Color` vertex attribute at line 48 |
| `coord` variable in scope at tint insertion | Outer loop variable (line 1072 in palette overload), accessible in inner vertex loop |
