# VoxelizerAPI Documentation

## Overview

`VoxelizerAPI` is a static utility class that converts 3D meshes into voxel representations. It supports both solid and surface voxelization, with optional texture color sampling.

**Location:** `PewPew/src/PewPew/Renderer/VoxelizerAPI.h`

---

## Table of Contents

- [Initialization](#initialization)
- [Data Structures](#data-structures)
- [Voxelization Methods](#voxelization-methods)
- [Mesh Creation Methods](#mesh-creation-methods)
- [Query Methods](#query-methods)
- [Helper Methods](#helper-methods)
- [Complete Usage Example](#complete-usage-example)
- [Performance Notes](#performance-notes)
- [VoxelGrid Reference](#voxelgrid-reference)

---

## Initialization

```cpp
// Call once at startup (e.g., in your Layer constructor)
PewPew::VoxelizerAPI::Init();

// Call at shutdown (optional - cleans up cached data)
PewPew::VoxelizerAPI::Shutdown();
```

---

## Data Structures

### VoxelizeSettings

Configuration for voxelization process.

```cpp
struct VoxelizeSettings
{
    int GridSize = 32;      // Voxels along longest mesh axis (8-128 recommended)
    float VoxelSize = 0.0f; // If > 0, use fixed voxel size instead of GridSize
    bool Solid = true;      // true = fill interior, false = surface only
    float Padding = 0.01f;  // Padding around mesh bounds
};
```

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `GridSize` | int | 32 | Number of voxels along the longest axis of the mesh |
| `VoxelSize` | float | 0.0f | Fixed voxel size in world units (overrides GridSize if > 0) |
| `Solid` | bool | true | Fill interior voxels or surface only |
| `Padding` | float | 0.01f | Extra space around mesh bounds |

### VoxelMeshData

Result of voxelization operations.

```cpp
struct VoxelMeshData
{
    Ref<Mesh> Mesh;              // Renderable cube mesh
    voxelizer::VoxelGrid Grid;   // Raw voxel data for queries
    uint64_t VoxelCount = 0;     // Number of filled voxels
};
```

| Field | Type | Description |
|-------|------|-------------|
| `Mesh` | `Ref<Mesh>` | Renderable mesh containing one cube per voxel |
| `Grid` | `voxelizer::VoxelGrid` | Raw voxel grid data for spatial queries |
| `VoxelCount` | `uint64_t` | Total number of filled voxels |

### TextureSampler

Helper class for sampling colors from textures (used internally).

```cpp
class TextureSampler
{
public:
    bool Load(const String& filePath);           // Load texture from file
    Vector3 Sample(const Vector2& uv) const;     // Point sampling
    Vector3 SampleBilinear(const Vector2& uv) const; // Bilinear filtering

    uint32_t GetWidth() const;
    uint32_t GetHeight() const;
    bool IsLoaded() const;
};
```

---

## Voxelization Methods

### VoxelizeFromFile

Loads a mesh file and voxelizes it with uniform white color.

```cpp
static VoxelMeshData VoxelizeFromFile(
    const String& filePath,
    const VoxelizeSettings& settings = {}
);
```

**Parameters:**
| Name | Type | Description |
|------|------|-------------|
| `filePath` | `const String&` | Path to mesh file (FBX, OBJ, GLTF supported) |
| `settings` | `VoxelizeSettings` | Voxelization configuration |

**Returns:** `VoxelMeshData` containing the voxelized mesh

**Example:**
```cpp
PewPew::VoxelizeSettings settings;
settings.GridSize = 32;
settings.Solid = true;

auto data = PewPew::VoxelizerAPI::VoxelizeFromFile("assets/models/Character.fbx", settings);
```

---

### VoxelizeFromFileWithColors

Loads a mesh and voxelizes it with colors sampled from a texture.

```cpp
static VoxelMeshData VoxelizeFromFileWithColors(
    const String& meshPath,
    const String& texturePath,
    const VoxelizeSettings& settings = {}
);
```

**Parameters:**
| Name | Type | Description |
|------|------|-------------|
| `meshPath` | `const String&` | Path to mesh file |
| `texturePath` | `const String&` | Path to texture file (PNG, JPG, TGA) |
| `settings` | `VoxelizeSettings` | Voxelization configuration |

**Returns:** `VoxelMeshData` with per-voxel colors from texture

**Example:**
```cpp
auto data = PewPew::VoxelizerAPI::VoxelizeFromFileWithColors(
    "assets/models/Character.fbx",
    "assets/textures/CharacterTexture.png",
    settings
);
```

---

### Voxelize

Voxelizes from vertex/index arrays when you already have mesh data loaded.

```cpp
static VoxelMeshData Voxelize(
    const std::vector<Vertex>& vertices,
    const std::vector<uint32_t>& indices,
    const VoxelizeSettings& settings = {}
);
```

**Parameters:**
| Name | Type | Description |
|------|------|-------------|
| `vertices` | `std::vector<Vertex>&` | Mesh vertex data |
| `indices` | `std::vector<uint32_t>&` | Triangle indices |
| `settings` | `VoxelizeSettings` | Voxelization configuration |

**Returns:** `VoxelMeshData` containing the voxelized mesh

---

### VoxelizeWithColors

Voxelizes from vertex/index arrays with texture color sampling.

```cpp
static VoxelMeshData VoxelizeWithColors(
    const std::vector<Vertex>& vertices,
    const std::vector<uint32_t>& indices,
    const String& texturePath,
    const VoxelizeSettings& settings = {}
);
```

**Parameters:**
| Name | Type | Description |
|------|------|-------------|
| `vertices` | `std::vector<Vertex>&` | Mesh vertex data (must include UV coordinates) |
| `indices` | `std::vector<uint32_t>&` | Triangle indices |
| `texturePath` | `const String&` | Path to texture file |
| `settings` | `VoxelizeSettings` | Voxelization configuration |

**Returns:** `VoxelMeshData` with per-voxel colors

---

## Mesh Creation Methods

### CreateMeshFromGrid

Creates a renderable mesh from a voxel grid.

```cpp
// With default white color
static Ref<Mesh> CreateMeshFromGrid(const voxelizer::VoxelGrid& grid);

// With uniform color
static Ref<Mesh> CreateMeshFromGrid(
    const voxelizer::VoxelGrid& grid,
    const Vector3& color
);
```

**Parameters:**
| Name | Type | Description |
|------|------|-------------|
| `grid` | `voxelizer::VoxelGrid&` | Voxel grid data |
| `color` | `Vector3` | RGB color (0.0 - 1.0) for all voxels |

**Returns:** `Ref<Mesh>` - Renderable mesh with one cube per filled voxel

---

### CreateColoredMeshFromGrid

Creates a mesh with individual colors per voxel.

```cpp
static Ref<Mesh> CreateColoredMeshFromGrid(
    const voxelizer::VoxelGrid& grid,
    const std::vector<Vector3>& voxelColors
);
```

**Parameters:**
| Name | Type | Description |
|------|------|-------------|
| `grid` | `voxelizer::VoxelGrid&` | Voxel grid data |
| `voxelColors` | `std::vector<Vector3>&` | One RGB color per filled voxel |

**Returns:** `Ref<Mesh>` - Renderable mesh with per-voxel vertex colors

**Note:** `voxelColors.size()` must equal the number of filled voxels in the grid.

---

## Query Methods

### GetVoxelPositions

Returns world-space center positions of all filled voxels.

```cpp
static std::vector<Vector3> GetVoxelPositions(const voxelizer::VoxelGrid& grid);
```

**Returns:** Vector of world-space positions for each filled voxel

**Example:**
```cpp
auto positions = PewPew::VoxelizerAPI::GetVoxelPositions(voxelData.Grid);
for (const auto& pos : positions)
{
    // Spawn particle at each voxel position
    SpawnParticle(pos);
}
```

---

### GetVoxelSize

Returns the size of each voxel in world units.

```cpp
static Vector3 GetVoxelSize(const voxelizer::VoxelGrid& grid);
```

**Returns:** `Vector3` - Size of a single voxel (x, y, z)

---

### IsPointInside

Checks if a world position is inside the voxelized volume.

```cpp
static bool IsPointInside(
    const voxelizer::VoxelGrid& grid,
    const Vector3& worldPos
);
```

**Parameters:**
| Name | Type | Description |
|------|------|-------------|
| `grid` | `voxelizer::VoxelGrid&` | Voxel grid data |
| `worldPos` | `Vector3` | World-space position to check |

**Returns:** `true` if the position is inside a filled voxel

**Example:**
```cpp
Vector3 playerPos = GetPlayerPosition();
if (PewPew::VoxelizerAPI::IsPointInside(voxelData.Grid, playerPos))
{
    // Player is colliding with voxelized mesh
    HandleCollision();
}
```

---

## Helper Methods

### CreateVoxelMaterial

Creates a simple PBR material suitable for voxel rendering.

```cpp
static Ref<Material> CreateVoxelMaterial(
    const Vector3& color = Vector3(0.8f, 0.8f, 0.8f)
);
```

**Parameters:**
| Name | Type | Default | Description |
|------|------|---------|-------------|
| `color` | `Vector3` | (0.8, 0.8, 0.8) | RGB albedo color |

**Returns:** `Ref<Material>` with roughness=0.7, metallic=0.0

**Example:**
```cpp
auto material = PewPew::VoxelizerAPI::CreateVoxelMaterial({0.8f, 0.2f, 0.2f}); // Red
material->SetRoughness(0.5f);
material->SetMetallic(0.1f);
```

---

### GetVoxelShader

Returns the default PBR shader for voxel rendering.

```cpp
static Ref<Shader> GetVoxelShader();
```

**Returns:** `Ref<Shader>` - Cached PBR shader instance

---

## Complete Usage Example

```cpp
#include <PewPew.h>

class VoxelLayer : public PewPew::Layer
{
public:
    VoxelLayer() : Layer("VoxelLayer")
    {
        // Initialize VoxelizerAPI
        PewPew::VoxelizerAPI::Init();

        // Configure voxelization
        PewPew::VoxelizeSettings settings;
        settings.GridSize = 48;
        settings.Solid = true;

        // Voxelize with texture colors
        m_VoxelData = PewPew::VoxelizerAPI::VoxelizeFromFileWithColors(
            "assets/models/Character.fbx",
            "assets/textures/Character.png",
            settings
        );

        // Setup rendering
        m_Shader = PewPew::Shader::Create("assets/shaders/PBR.glsl");
        m_Material = PewPew::VoxelizerAPI::CreateVoxelMaterial({1.0f, 1.0f, 1.0f});

        // Setup camera
        m_CameraController = PewPew::PerspectiveCameraController(45.0f, 16.0f/9.0f, 0.1f, 100.0f);
    }

    void OnUpdate(PewPew::Timestep ts) override
    {
        // Update camera
        m_CameraController.OnUpdate(ts);

        // Clear screen
        PewPew::RenderCommand::SetClearColor({0.1f, 0.1f, 0.15f, 1.0f});
        PewPew::RenderCommand::Clear();

        // Render voxelized mesh
        PewPew::Renderer3D::BeginScene(
            m_CameraController.GetCamera(),
            m_CameraController.GetCamera().GetPosition()
        );

        if (m_VoxelData.Mesh)
        {
            Mat4 transform = glm::scale(Mat4(1.0f), Vector3(0.1f));
            PewPew::Renderer3D::Submit(m_Shader, m_Material, m_VoxelData.Mesh, transform);
        }

        PewPew::Renderer3D::EndScene();
    }

    void OnImGuiRender() override
    {
        ImGui::Begin("Voxel Info");
        ImGui::Text("Voxel Count: %llu", m_VoxelData.VoxelCount);
        ImGui::Text("Grid: (%d, %d, %d)",
            m_VoxelData.Grid.size.x,
            m_VoxelData.Grid.size.y,
            m_VoxelData.Grid.size.z);
        ImGui::End();
    }

    void OnEvent(PewPew::Event& event) override
    {
        m_CameraController.OnEvent(event);
    }

private:
    PewPew::VoxelMeshData m_VoxelData;
    PewPew::Ref<PewPew::Shader> m_Shader;
    PewPew::Ref<PewPew::Material> m_Material;
    PewPew::PerspectiveCameraController m_CameraController;
};
```

---

## Performance Notes

### Grid Size vs Performance

| Grid Size | Approx. Voxels (Solid) | Triangles | Use Case |
|-----------|------------------------|-----------|----------|
| 16 | ~4,000 | ~48K | Real-time, low detail |
| 32 | ~32,000 | ~384K | Balanced (recommended) |
| 64 | ~260,000 | ~3.1M | High detail |
| 128 | ~2,000,000 | ~25M | Maximum detail, may lag |

### Optimization Tips

1. **Use `Solid = false`** for hollow objects to reduce voxel count
2. **Cache `VoxelMeshData`** - don't re-voxelize every frame
3. **Lower `GridSize`** for real-time re-voxelization
4. **Pre-voxelize at load time** rather than during gameplay

### Memory Usage

Each voxel generates:
- 24 vertices (4 per face × 6 faces)
- 36 indices (6 per face × 6 faces)
- ~1.4 KB per voxel

---

## VoxelGrid Reference

The `voxelizer::VoxelGrid` struct provides direct access to voxel data:

### Properties

```cpp
glm::ivec3 size;       // Grid dimensions (x, y, z)
glm::vec3 origin;      // World-space origin (lower bound)
glm::vec3 voxelSize;   // Size of each voxel in world units
uint64_t totalVoxels;  // Total grid cells (size.x * size.y * size.z)
```

### Methods

```cpp
// Check if voxel is filled
bool IsFilled(int x, int y, int z) const;
bool IsFilled(const glm::ivec3& coord) const;
bool IsFilled(uint64_t index) const;

// Set/clear voxels
void SetFilled(int x, int y, int z);
void Clear(int x, int y, int z);

// Coordinate conversions
uint64_t CoordToIndex(int x, int y, int z) const;
glm::ivec3 IndexToCoord(uint64_t index) const;

// World/voxel space conversions
glm::ivec3 WorldToVoxel(const glm::vec3& worldPos) const;
glm::vec3 VoxelToWorld(const glm::ivec3& voxelCoord) const;
glm::vec3 VoxelCenterToWorld(const glm::ivec3& voxelCoord) const;

// Bounds checking
bool IsValidCoord(int x, int y, int z) const;
bool IsValidCoord(const glm::ivec3& coord) const;

// Queries
uint64_t CountFilled() const;
std::vector<glm::ivec3> GetFilledCoords() const;
std::vector<glm::vec3> GetFilledPositions() const;
```

### Example: Custom Voxel Iteration

```cpp
const auto& grid = voxelData.Grid;

// Iterate all voxels
for (int x = 0; x < grid.size.x; x++)
{
    for (int y = 0; y < grid.size.y; y++)
    {
        for (int z = 0; z < grid.size.z; z++)
        {
            if (grid.IsFilled(x, y, z))
            {
                glm::vec3 worldPos = grid.VoxelCenterToWorld({x, y, z});
                // Do something with this voxel
            }
        }
    }
}

// Or use the helper method
for (const auto& coord : grid.GetFilledCoords())
{
    glm::vec3 worldPos = grid.VoxelCenterToWorld(coord);
    // Process voxel
}
```

---

## See Also

- `PewPew::Renderer3D` - For rendering voxelized meshes
- `PewPew::Material` - For customizing voxel appearance
- `PewPew::Mesh` - Base mesh class
