# Renderer Guide

This document covers the 3D rendering system, including shaders, materials, meshes, textures, and cameras.

## Table of Contents

- [Renderer Architecture](#renderer-architecture)
- [Render Commands](#render-commands)
- [Renderer3D](#renderer3d)
- [Shaders](#shaders)
- [Textures](#textures)
- [Materials](#materials)
- [Meshes](#meshes)
- [Vertex Buffers](#vertex-buffers)
- [Cameras](#cameras)
- [Camera Controllers](#camera-controllers)
- [Lighting](#lighting)
- [Complete Example](#complete-example)

---

## Renderer Architecture

The rendering system uses a layered abstraction:

```
┌─────────────────────────────────────────┐
│            Renderer3D / Renderer        │  High-level API
├─────────────────────────────────────────┤
│              RenderCommand              │  Command buffer
├─────────────────────────────────────────┤
│              RendererAPI                │  Abstraction
├─────────────────────────────────────────┤
│           OpenGLRendererAPI             │  Implementation
├─────────────────────────────────────────┤
│         OpenGL (via Glad/GLFW)          │  Graphics API
└─────────────────────────────────────────┘
```

### Key Components

| Component | Responsibility |
|-----------|----------------|
| `Renderer3D` | Scene management, PBR lighting, 3D submissions |
| `Renderer` | Basic 2D rendering (less commonly used) |
| `RenderCommand` | Low-level draw calls (clear, viewport, draw) |
| `RendererAPI` | Platform abstraction interface |

---

## Render Commands

Low-level rendering operations through `RenderCommand`.

### Location
`PewPew/src/PewPew/Renderer/Core/RenderCommand.h`

### API

```cpp
class RenderCommand
{
public:
    static void Init();
    static void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height);
    static void SetClearColor(const Vector4& color);
    static void Clear();
    static void DrawIndexed(const Ref<VertexArray>& vertexArray);
};
```

### Usage

```cpp
void OnUpdate(PewPew::Timestep ts)
{
    // Set clear color (RGBA, 0-1 range)
    PewPew::RenderCommand::SetClearColor({0.1f, 0.1f, 0.15f, 1.0f});

    // Clear color and depth buffers
    PewPew::RenderCommand::Clear();

    // Viewport (usually handled by window resize)
    PewPew::RenderCommand::SetViewport(0, 0, width, height);

    // ... render scene
}
```

---

## Renderer3D

The main 3D rendering interface with PBR support.

### Location
`PewPew/src/PewPew/Renderer/Core/Renderer3D.h`

### API

```cpp
class Renderer3D
{
public:
    static void Init();
    static void Shutdown();

    static void BeginScene(const Camera& camera, const Vector3& cameraPosition);
    static void EndScene();

    static void Submit(const Ref<Shader>& shader,
                       const Ref<Material>& material,
                       const Ref<Mesh>& mesh,
                       const Mat4& transform = Mat4(1.0f));

    // Lighting
    static void SetDirectionalLight(const Vector3& direction,
                                    const Vector3& color,
                                    float intensity);
    static void SetAmbientLight(const Vector3& color);
};
```

### Render Flow

```cpp
void OnUpdate(PewPew::Timestep ts)
{
    // 1. Clear screen
    PewPew::RenderCommand::SetClearColor({0.1f, 0.1f, 0.15f, 1.0f});
    PewPew::RenderCommand::Clear();

    // 2. Begin scene with camera
    auto& camera = m_CameraController->GetCamera();
    PewPew::Renderer3D::BeginScene(camera, camera.GetPosition());

    // 3. Configure lighting (optional, has defaults)
    PewPew::Renderer3D::SetDirectionalLight(
        {-0.5f, -1.0f, -0.3f},  // Direction
        {1.0f, 1.0f, 1.0f},     // Color
        1.0f                     // Intensity
    );

    // 4. Submit meshes
    glm::mat4 transform = glm::translate(glm::mat4(1.0f), {0, 0, -5});
    PewPew::Renderer3D::Submit(m_Shader, m_Material, m_Mesh, transform);

    // 5. End scene
    PewPew::Renderer3D::EndScene();
}
```

---

## Shaders

GLSL shaders with a single-file format.

### Location
`PewPew/src/PewPew/Renderer/Resources/Shader.h`

### Creating Shaders

```cpp
// From file (recommended)
Ref<Shader> shader = PewPew::Shader::Create("assets/shaders/PBR.glsl");

// From source strings
Ref<Shader> shader = PewPew::Shader::Create(
    "MyShader",
    vertexSource,
    fragmentSource
);
```

### Shader File Format

Use `#type` directives to separate stages:

```glsl
#type vertex
#version 450 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TexCoords;
layout(location = 3) in vec3 a_Tangent;
layout(location = 4) in vec3 a_Bitangent;
layout(location = 5) in vec3 a_Color;

uniform mat4 u_ViewProjection;
uniform mat4 u_Transform;

out vec3 v_Position;
out vec3 v_Normal;
out vec2 v_TexCoords;
out vec3 v_Color;
out mat3 v_TBN;

void main()
{
    vec4 worldPos = u_Transform * vec4(a_Position, 1.0);
    v_Position = worldPos.xyz;

    mat3 normalMatrix = mat3(transpose(inverse(u_Transform)));
    v_Normal = normalMatrix * a_Normal;

    vec3 T = normalize(normalMatrix * a_Tangent);
    vec3 B = normalize(normalMatrix * a_Bitangent);
    vec3 N = normalize(v_Normal);
    v_TBN = mat3(T, B, N);

    v_TexCoords = a_TexCoords;
    v_Color = a_Color;

    gl_Position = u_ViewProjection * worldPos;
}

#type fragment
#version 450 core

layout(location = 0) out vec4 o_Color;

in vec3 v_Position;
in vec3 v_Normal;
in vec2 v_TexCoords;
in vec3 v_Color;
in mat3 v_TBN;

// Material uniforms
uniform sampler2D u_AlbedoMap;
uniform sampler2D u_NormalMap;
uniform sampler2D u_MetallicMap;
uniform sampler2D u_RoughnessMap;

uniform vec3 u_Albedo;
uniform float u_Metallic;
uniform float u_Roughness;
uniform float u_SmoothShading;

uniform bool u_HasAlbedoMap;
uniform bool u_HasNormalMap;
uniform bool u_HasMetallicMap;
uniform bool u_HasRoughnessMap;

// Scene uniforms
uniform vec3 u_CameraPosition;
uniform vec3 u_LightDirection;
uniform vec3 u_LightColor;
uniform float u_LightIntensity;
uniform vec3 u_AmbientColor;

void main()
{
    // Sample textures or use defaults
    vec3 albedo = u_HasAlbedoMap
        ? texture(u_AlbedoMap, v_TexCoords).rgb
        : u_Albedo;

    vec3 normal = v_Normal;
    if (u_HasNormalMap)
    {
        normal = texture(u_NormalMap, v_TexCoords).rgb * 2.0 - 1.0;
        normal = normalize(v_TBN * normal);
    }

    // Apply vertex color
    albedo *= v_Color;

    // Simple diffuse lighting
    vec3 lightDir = normalize(-u_LightDirection);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * u_LightColor * u_LightIntensity;

    vec3 result = (u_AmbientColor + diffuse) * albedo;
    o_Color = vec4(result, 1.0);
}
```

### Setting Uniforms

```cpp
// Uniforms are typically set by Material and Renderer3D
// For custom uniforms:
m_Shader->Bind();
m_Shader->SetInt("u_Texture", 0);
m_Shader->SetFloat("u_Time", time);
m_Shader->SetFloat3("u_Color", {1.0f, 0.5f, 0.2f});
m_Shader->SetMat4("u_Transform", transform);
```

### Shader Library

Manage multiple shaders:

```cpp
PewPew::ShaderLibrary shaderLib;

// Load shaders
shaderLib.Load("assets/shaders/PBR.glsl");
shaderLib.Load("Flat", "assets/shaders/Flat.glsl");

// Retrieve later
auto pbrShader = shaderLib.Get("PBR");
auto flatShader = shaderLib.Get("Flat");
```

---

## Textures

2D texture loading and binding.

### Location
`PewPew/src/PewPew/Renderer/Resources/Texture.h`

### Creating Textures

```cpp
// Load from file (PNG, JPG, TGA)
Ref<Texture2D> texture = PewPew::Texture2D::Create("assets/textures/albedo.png");
```

### Texture Slots

```cpp
// Bind to texture unit
texture->Bind(0);  // GL_TEXTURE0
normalMap->Bind(1);  // GL_TEXTURE1
```

### Supported Formats

- PNG (recommended, lossless)
- JPG
- TGA
- BMP

---

## Materials

PBR material properties and texture maps.

### Location
`PewPew/src/PewPew/Renderer/Resources/Material.h`

### Creating Materials

```cpp
Ref<Material> material = PewPew::CreateRef<PewPew::Material>();

// Set texture maps
material->SetAlbedoMap(albedoTexture);
material->SetNormalMap(normalTexture);
material->SetMetallicMap(metallicTexture);
material->SetRoughnessMap(roughnessTexture);

// Set scalar fallbacks (used when no texture)
material->SetAlbedo({0.8f, 0.2f, 0.2f});  // Red
material->SetMetallic(0.0f);               // Non-metallic
material->SetRoughness(0.5f);              // Medium roughness
```

### Material Properties

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `Albedo` | `Vector3` | `(1, 1, 1)` | Base color |
| `Metallic` | `float` | `0.0` | Metalness (0-1) |
| `Roughness` | `float` | `0.5` | Surface roughness (0-1) |
| `SmoothShading` | `float` | `1.0` | Normal smoothing factor |

### Texture Maps

| Map | Shader Slot | Purpose |
|-----|-------------|---------|
| Albedo | 0 | Base color texture |
| Normal | 1 | Surface detail normals |
| Metallic | 2 | Per-pixel metalness |
| Roughness | 3 | Per-pixel roughness |

---

## Meshes

3D model loading and rendering.

### Location
`PewPew/src/PewPew/Renderer/Resources/Mesh.h`

### Loading Meshes

```cpp
// Load from file (FBX, OBJ, GLTF)
Ref<Mesh> mesh = PewPew::Mesh::Load("assets/models/character.fbx");
```

### Supported Formats

Via Assimp:
- FBX (recommended)
- OBJ
- GLTF / GLB
- DAE (Collada)
- 3DS
- And many more

### Vertex Format

```cpp
struct Vertex
{
    Vector3 Position;
    Vector3 Normal;
    Vector2 TexCoords;
    Vector3 Tangent;
    Vector3 Bitangent;
    Vector3 Color = Vector3(1.0f);  // Vertex color
};
```

### Programmatic Mesh Creation

```cpp
std::vector<PewPew::Vertex> vertices = {
    // Position, Normal, TexCoords, Tangent, Bitangent, Color
    {{-0.5f, -0.5f, 0.0f}, {0, 0, 1}, {0, 0}, {1, 0, 0}, {0, 1, 0}, {1, 1, 1}},
    {{ 0.5f, -0.5f, 0.0f}, {0, 0, 1}, {1, 0}, {1, 0, 0}, {0, 1, 0}, {1, 1, 1}},
    {{ 0.5f,  0.5f, 0.0f}, {0, 0, 1}, {1, 1}, {1, 0, 0}, {0, 1, 0}, {1, 1, 1}},
    {{-0.5f,  0.5f, 0.0f}, {0, 0, 1}, {0, 1}, {1, 0, 0}, {0, 1, 0}, {1, 1, 1}},
};

std::vector<uint32_t> indices = {
    0, 1, 2,
    2, 3, 0
};

Ref<Mesh> quad = PewPew::CreateRef<PewPew::Mesh>(vertices, indices);
```

---

## Vertex Buffers

Low-level buffer management.

### Location
`PewPew/src/PewPew/Renderer/Resources/Buffer.h`

### Buffer Layout

```cpp
PewPew::BufferLayout layout = {
    { PewPew::ShaderDataType::Float3, "a_Position" },
    { PewPew::ShaderDataType::Float3, "a_Normal" },
    { PewPew::ShaderDataType::Float2, "a_TexCoords" },
    { PewPew::ShaderDataType::Float3, "a_Tangent" },
    { PewPew::ShaderDataType::Float3, "a_Bitangent" },
    { PewPew::ShaderDataType::Float3, "a_Color" }
};
```

### Shader Data Types

| Type | Size | Components |
|------|------|------------|
| `Float` | 4 | 1 |
| `Float2` | 8 | 2 |
| `Float3` | 12 | 3 |
| `Float4` | 16 | 4 |
| `Mat3` | 36 | 9 |
| `Mat4` | 64 | 16 |
| `Int` | 4 | 1 |
| `Bool` | 1 | 1 |

### Manual Buffer Creation

```cpp
// Create vertex buffer
float vertices[] = { /* ... */ };
auto vb = PewPew::VertexBuffer::Create(vertices, sizeof(vertices));
vb->SetLayout(layout);

// Create index buffer
uint32_t indices[] = { 0, 1, 2, 2, 3, 0 };
auto ib = PewPew::IndexBuffer::Create(indices, 6);

// Create vertex array
auto va = PewPew::VertexArray::Create();
va->AddVertexBuffer(vb);
va->SetIndexBuffer(ib);
```

---

## Cameras

Camera implementations for 3D viewing.

### Location
- `PewPew/src/PewPew/Renderer/Camera/Camera.h`
- `PewPew/src/PewPew/Renderer/Camera/PerspectiveCamera.h`

### Camera Base Class

```cpp
class Camera
{
public:
    virtual const Mat4& GetProjectionMatrix() const = 0;
    virtual const Mat4& GetViewMatrix() const = 0;
    virtual const Mat4& GetViewProjectionMatrix() const = 0;
};
```

### Perspective Camera

```cpp
// Create camera
PewPew::PerspectiveCamera camera(
    45.0f,    // FOV in degrees
    16.0f/9.0f,  // Aspect ratio
    0.1f,     // Near plane
    1000.0f   // Far plane
);

// Position and rotation
camera.SetPosition({0.0f, 5.0f, 10.0f});
camera.SetRotation(-15.0f, 0.0f);  // Pitch, Yaw (degrees)

// Get direction vectors
Vector3 forward = camera.GetForwardDirection();
Vector3 right = camera.GetRightDirection();
Vector3 up = camera.GetUpDirection();
```

---

## Camera Controllers

Pre-built camera control logic.

### Location
`PewPew/src/PewPew/Renderer/Camera/PerspectiveCameraController.h`

### Creating a Controller

```cpp
// First-person style controller
auto controller = PewPew::CreateScope<PewPew::PerspectiveCameraController>(
    45.0f,       // FOV
    16.0f/9.0f,  // Aspect ratio
    0.1f,        // Near
    1000.0f,     // Far
    true         // Enable rotation
);
```

### Using the Controller

```cpp
void OnUpdate(PewPew::Timestep ts) override
{
    m_CameraController->OnUpdate(ts);

    // Access camera for rendering
    auto& camera = m_CameraController->GetCamera();
    PewPew::Renderer3D::BeginScene(camera, camera.GetPosition());
    // ...
}

void OnEvent(PewPew::Event& e) override
{
    m_CameraController->OnEvent(e);
}
```

### Default Controls

| Input | Action |
|-------|--------|
| **W** | Move forward |
| **S** | Move backward |
| **A** | Move left |
| **D** | Move right |
| **Mouse** | Look around |
| **Scroll** | Zoom (FOV) |

### Customizing Movement

```cpp
// Access underlying camera
PewPew::PerspectiveCamera& camera = m_CameraController->GetCamera();

// Set initial position
camera.SetPosition({0, 10, 20});
camera.SetRotation(-20.0f, 0.0f);

// Disable/enable controller
m_CameraController->SetEnabled(false);
```

---

## Lighting

The Renderer3D provides basic directional lighting.

### Setting Lights

```cpp
// Directional light (sun)
PewPew::Renderer3D::SetDirectionalLight(
    {-0.5f, -1.0f, -0.3f},  // Direction (pointing toward light source)
    {1.0f, 0.95f, 0.9f},    // Warm white color
    1.2f                     // Intensity multiplier
);

// Ambient light
PewPew::Renderer3D::SetAmbientLight({0.03f, 0.03f, 0.05f});
```

### Default Values

| Property | Default |
|----------|---------|
| Light Direction | `(-0.5, -1.0, -0.3)` |
| Light Color | `(1.0, 1.0, 1.0)` |
| Light Intensity | `1.0` |
| Ambient Color | `(0.03, 0.03, 0.03)` |

---

## Complete Example

A full rendering setup:

```cpp
class RenderLayer : public PewPew::Layer
{
public:
    RenderLayer() : Layer("RenderLayer") {}

    void OnAttach() override
    {
        // Load shader
        m_Shader = PewPew::Shader::Create("assets/shaders/PBR.glsl");

        // Load mesh
        m_Mesh = PewPew::Mesh::Load("assets/models/helmet.fbx");

        // Create material with textures
        m_Material = PewPew::CreateRef<PewPew::Material>();
        m_Material->SetAlbedoMap(
            PewPew::Texture2D::Create("assets/textures/helmet_albedo.png"));
        m_Material->SetNormalMap(
            PewPew::Texture2D::Create("assets/textures/helmet_normal.png"));
        m_Material->SetRoughnessMap(
            PewPew::Texture2D::Create("assets/textures/helmet_roughness.png"));
        m_Material->SetMetallicMap(
            PewPew::Texture2D::Create("assets/textures/helmet_metallic.png"));

        // Setup camera
        float aspect = 1280.0f / 720.0f;
        m_CameraController = PewPew::CreateScope<PewPew::PerspectiveCameraController>(
            45.0f, aspect, 0.1f, 1000.0f);
        m_CameraController->GetCamera().SetPosition({0, 0, 5});
    }

    void OnUpdate(PewPew::Timestep ts) override
    {
        // Update camera
        m_CameraController->OnUpdate(ts);

        // Clear
        PewPew::RenderCommand::SetClearColor({0.1f, 0.1f, 0.15f, 1.0f});
        PewPew::RenderCommand::Clear();

        // Render scene
        auto& camera = m_CameraController->GetCamera();
        PewPew::Renderer3D::BeginScene(camera, camera.GetPosition());

        // Configure lighting
        PewPew::Renderer3D::SetDirectionalLight(
            {-0.5f, -1.0f, -0.3f},
            {1.0f, 1.0f, 1.0f},
            1.0f
        );

        // Rotate model over time
        static float rotation = 0.0f;
        rotation += ts * 30.0f;  // 30 degrees per second

        glm::mat4 transform = glm::rotate(
            glm::mat4(1.0f),
            glm::radians(rotation),
            {0, 1, 0}
        );

        // Submit
        PewPew::Renderer3D::Submit(m_Shader, m_Material, m_Mesh, transform);

        PewPew::Renderer3D::EndScene();
    }

    void OnImGuiRender() override
    {
        ImGui::Begin("Render Settings");

        // Material controls
        if (ImGui::CollapsingHeader("Material"))
        {
            glm::vec3 albedo = m_Material->GetAlbedo();
            if (ImGui::ColorEdit3("Albedo", &albedo.x))
                m_Material->SetAlbedo(albedo);

            float roughness = m_Material->GetRoughness();
            if (ImGui::SliderFloat("Roughness", &roughness, 0.0f, 1.0f))
                m_Material->SetRoughness(roughness);

            float metallic = m_Material->GetMetallic();
            if (ImGui::SliderFloat("Metallic", &metallic, 0.0f, 1.0f))
                m_Material->SetMetallic(metallic);
        }

        ImGui::End();
    }

    void OnEvent(PewPew::Event& e) override
    {
        m_CameraController->OnEvent(e);
    }

private:
    PewPew::Ref<PewPew::Shader> m_Shader;
    PewPew::Ref<PewPew::Mesh> m_Mesh;
    PewPew::Ref<PewPew::Material> m_Material;
    PewPew::Scope<PewPew::PerspectiveCameraController> m_CameraController;
};
```

---

## Next Steps

- [Voxelizer API](./VoxelizerAPI.md) - Convert meshes to voxels
- [API Reference](./API_REFERENCE.md) - Complete API listing
