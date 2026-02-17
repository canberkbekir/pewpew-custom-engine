# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build System

This is a C++17 Windows game engine using Premake5 to generate Visual Studio 2022 solutions.

**Generate project files:**
```batch
GenerateProject.bat
```
This runs `vendor\bin\premake\premake5.exe vs2022` to regenerate the `.sln` and `.vcxproj` files.

**Build via command line (MSBuild):**
```batch
msbuild CBEngine.sln /p:Configuration=Debug /p:Platform=x64
msbuild CBEngine.sln /p:Configuration=Release /p:Platform=x64
```

**Configurations:** Debug, Release, Dist (distribution)

**Output directories:**
- Binaries: `bin/{Config}-windows-x64/{ProjectName}/`
- Intermediates: `bin-int/{Config}-windows-x64/{ProjectName}/`

## Architecture

### Project Structure

```
CBEngine/                        # Core engine (static library)
├── src/
│   ├── cbpch.h/cpp              # Precompiled header
│   ├── CBEngine.h               # Main include header for clients
│   ├── CBEngine/                # Engine source code
│   │   ├── Core/                # Core systems
│   │   │   ├── Application.h/cpp    # Main application class
│   │   │   ├── Core.h               # Macros, smart pointers, assertions
│   │   │   ├── EntryPoint.h         # main() definition
│   │   │   ├── Layer.h/cpp          # Layer base class
│   │   │   ├── LayerStack.h/cpp     # Layer management
│   │   │   ├── Log.h/cpp            # Logging system
│   │   │   ├── LogBuffer.h/cpp      # Log buffering
│   │   │   ├── BufferedLogSink.h    # spdlog sink for buffered logs
│   │   │   ├── String.h             # String type alias
│   │   │   ├── TimeStep.h           # Frame delta time
│   │   │   ├── UUID.h               # Unique identifier for assets/entities
│   │   │   └── Window.h             # Window abstraction
│   │   │
│   │   ├── Input/               # Input handling
│   │   │   ├── Input.h              # Input polling API
│   │   │   ├── KeyCodes.h           # Keyboard key definitions
│   │   │   └── MouseButtonCodes.h   # Mouse button definitions
│   │   │
│   │   ├── Events/              # Event system
│   │   │   ├── Event.h              # Base event class
│   │   │   ├── ApplicationEvent.h   # Window events
│   │   │   ├── KeyEvent.h           # Keyboard events
│   │   │   ├── MouseEvent.h         # Mouse events
│   │   │   ├── SceneEvent.h         # Scene/entity events
│   │   │   ├── AssetEvent.h         # Asset lifecycle events
│   │   │   └── EditorEvent.h        # Editor mode events
│   │   │
│   │   ├── Renderer/            # Rendering system
│   │   │   ├── Core/                # Renderer core
│   │   │   │   ├── Renderer.h/cpp       # Basic renderer
│   │   │   │   ├── Renderer3D.h/cpp     # 3D PBR renderer
│   │   │   │   ├── RenderCommand.h/cpp  # Low-level commands
│   │   │   │   ├── RendererAPI.h/cpp    # Graphics API abstraction
│   │   │   │   ├── Framebuffer.h/cpp    # Framebuffer abstraction
│   │   │   │   ├── ShaderUniforms.h     # Uniform helpers
│   │   │   │   └── GraphicsContext.h/cpp
│   │   │   ├── Camera/              # Camera implementations
│   │   │   │   ├── Camera.h
│   │   │   │   ├── CameraController.h
│   │   │   │   ├── PerspectiveCamera.h/cpp
│   │   │   │   ├── OrthographicCamera.h/cpp
│   │   │   │   └── PerspectiveCameraController.h/cpp
│   │   │   └── Resources/           # GPU resources
│   │   │       ├── Buffer.h/cpp         # Vertex/Index buffers
│   │   │       ├── VertexArray.h/cpp    # VAO abstraction
│   │   │       ├── Shader.h/cpp         # Shader programs
│   │   │       ├── Texture.h/cpp        # Texture loading
│   │   │       ├── Mesh.h/cpp           # 3D model loading
│   │   │       └── Material.h/cpp       # PBR materials
│   │   │
│   │   ├── Scene/               # ECS and scene management
│   │   │   ├── Scene.h/cpp          # Scene (entity registry wrapper)
│   │   │   ├── Entity.h/cpp         # Entity handle class
│   │   │   ├── SceneManager.h/cpp   # Multi-scene management
│   │   │   ├── SceneSerializer.h/cpp # YAML scene save/load
│   │   │   └── ComponentRegistry.h  # Compile-time component registration
│   │   │
│   │   ├── Components/          # ECS components
│   │   │   ├── Components.h             # Umbrella include
│   │   │   ├── CoreComponents.h         # IDComponent, TagComponent
│   │   │   ├── TransformComponent.h     # Transform with parent/child hierarchy
│   │   │   ├── MeshRendererComponent.h  # Mesh + material rendering
│   │   │   ├── VoxelRendererComponent.h # Voxelized mesh rendering
│   │   │   └── DirectionalLightComponent.h # Directional light
│   │   │
│   │   ├── Systems/             # ECS systems
│   │   │   ├── TransformSystem.h/cpp    # World transform computation
│   │   │   └── RendererSystem.h/cpp     # Scene rendering
│   │   │
│   │   ├── Asset/               # Asset management
│   │   │   ├── Asset.h              # Base asset class
│   │   │   ├── AssetHandle.h        # UUID-based asset handle
│   │   │   ├── AssetManager.h/cpp   # Central asset registry/loader
│   │   │   ├── AssetMetadata.h      # Asset metadata (path, type, UUID)
│   │   │   ├── AssetRegistry.h/cpp  # UUID-to-metadata mapping
│   │   │   ├── ProcessedMeshAsset.h # Cached processed mesh data
│   │   │   └── VoxelTextureAsset.h  # Voxel texture data asset
│   │   │
│   │   ├── Selection/           # Selection system
│   │   │   ├── Selection.h/cpp      # Current selection state
│   │   │   └── Selectable.h         # Selectable interface
│   │   │
│   │   ├── FileWatcher/         # File change monitoring
│   │   │   └── FileWatcher.h/cpp    # Directory watching for hot-reload
│   │   │
│   │   ├── Utils/               # Utility systems
│   │   │   ├── VoxelizerAPI.h/cpp       # Mesh voxelization
│   │   │   ├── VoxelPalette.h           # Voxel color palette
│   │   │   ├── VoxelMaterialType.h      # Voxel material type enum
│   │   │   ├── VoxelizationTask.h       # Async voxelization task
│   │   │   ├── BinaryIO.h              # Binary file read/write
│   │   │   ├── FileDialogs.h/cpp        # Native file open/save dialogs
│   │   │   └── YAMLHelpers.h            # YAML serialization utilities
│   │   │
│   │   ├── Debug/               # Debug tools
│   │   │   └── Instrumentor.h       # Profiling system
│   │   │
│   │   ├── ImGui/               # ImGui integration
│   │   │   ├── ImGuiLayer.h/cpp
│   │   │   ├── ImGuiUtils.h/cpp
│   │   │   └── ImGuiBuild.cpp
│   │   │
│   │   └── Math/                # Math utilities
│   │       └── CoreMath.h           # GLM type aliases
│   │
│   └── Platform/                # Platform implementations
│       ├── Windows/                 # Windows + GLFW
│       │   ├── WindowsWindow.h/cpp
│       │   └── WindowsInput.h/cpp
│       └── OpenGL/                  # OpenGL backend
│           ├── OpenGLContext.h/cpp
│           ├── OpenGLRendererAPI.h/cpp
│           ├── OpenGLShader.h/cpp
│           ├── OpenGLTexture.h/cpp
│           ├── OpenGLBuffer.h/cpp
│           ├── OpenGLFramebuffer.h/cpp
│           └── OpenGLVertexArray.h/cpp
│
└── vendor/                      # Third-party dependencies

CBEngine-Editor/                 # Editor application
├── src/
│   ├── EditorLayer.h/cpp        # Main editor layer
│   ├── Panels/                  # Editor panels (viewport, hierarchy, etc.)
│   └── Widgets/                 # Editor widgets (component views, editors)
└── assets/
    ├── models/                  # 3D models (FBX, OBJ, GLTF)
    ├── textures/                # Images (PNG, JPG, TGA)
    ├── shaders/                 # GLSL shaders
    └── materials/               # Material files (.mat)

Sandbox/                         # Test application (minimal)
├── src/
│   └── SandboxApp.cpp
└── assets/
    ├── models/
    ├── textures/
    └── shaders/
```

### Core Systems

**Application Lifecycle** (`CBEngine/src/CBEngine/Core/Application.h`):
- `Application::Run()` is the main loop: process events -> update layers -> render ImGui -> swap buffers
- Applications are created via `CreateApplication()` defined by the client
- Entry point is in `Core/EntryPoint.h` which defines `main()`

**Layer System** (`CBEngine/src/CBEngine/Core/Layer.h`, `LayerStack.h`):
- Game logic is organized into Layers with `OnUpdate()`, `OnEvent()`, `OnImGuiRender()` methods
- Layers are pushed to the Application's LayerStack

**Event System** (`CBEngine/src/CBEngine/Events/`):
- Events propagate through layers via `OnEvent()` callback
- Event types: KeyEvent, MouseEvent, ApplicationEvent, WindowCloseEvent, SceneEvent, AssetEvent
- Uses EventDispatcher pattern for type-safe event handling

**Input System** (`CBEngine/src/CBEngine/Input/`):
- `Input::IsKeyPressed()`, `Input::IsMouseButtonPressed()` for polling
- Key codes in `KeyCodes.h`, mouse buttons in `MouseButtonCodes.h`

**Renderer** (`CBEngine/src/CBEngine/Renderer/`):
- `Renderer3D` handles 3D scene rendering with PBR support
- `RendererAPI` abstracts graphics API (OpenGL implementation in `Platform/OpenGL/`)
- `RenderCommand` provides low-level draw calls

**Resources** (`CBEngine/src/CBEngine/Renderer/Resources/`):
- `Mesh` - 3D model loading via Assimp (FBX, OBJ, GLTF)
- `Shader` - GLSL shader compilation and uniform management
- `Texture` - Image loading via stb_image
- `Material` - PBR material properties (albedo, normal, roughness, metallic)

**Scene/ECS** (`CBEngine/src/CBEngine/Scene/`, `Components/`, `Systems/`):
- Uses entt for ECS. Scene wraps an entt registry.
- Components: TransformComponent (with parent/child hierarchy), MeshRendererComponent, VoxelRendererComponent, DirectionalLightComponent
- Systems: TransformSystem (world matrix computation), RendererSystem (scene drawing)
- SceneSerializer handles YAML save/load via ComponentRegistry fold-expression pattern

**Asset System** (`CBEngine/src/CBEngine/Asset/`):
- `AssetManager` provides UUID-based asset loading and caching
- `AssetRegistry` maps UUIDs to file paths via `.meta` sidecar files

**Camera** (`CBEngine/src/CBEngine/Renderer/Camera/`):
- `PerspectiveCamera`, `OrthographicCamera` with view/projection matrices
- `PerspectiveCameraController` for WASD + mouse control

**Utilities** (`CBEngine/src/CBEngine/Utils/`):
- `VoxelizerAPI` - Convert meshes to voxel representations
- `FileDialogs` - Native open/save file dialogs
- `BinaryIO` - Binary asset serialization

### Memory Management

Uses custom smart pointer aliases defined in `Core/Core.h`:
- `Scope<T>` = `std::unique_ptr<T>` (use `CreateScope<T>()`)
- `Ref<T>` = `std::shared_ptr<T>` (use `CreateRef<T>()`)

### Platform Abstraction

- `Platform/Windows/` - GLFW-based window and input implementation
- `Platform/OpenGL/` - OpenGL renderer implementation
- Platform macros: `CB_PLATFORM_WINDOWS`, `CB_DEBUG`, `CB_RELEASE`, `CB_DIST`

## Dependencies

**Vendor libraries** (in `CBEngine/vendor/`):
- GLFW - Window management
- Glad - OpenGL loader
- ImGui - Debug UI
- ImGuizmo - Gizmo rendering
- glm - Math library
- spdlog - Logging
- entt - Entity Component System
- yaml-cpp - YAML serialization

**Prebuilt** (in `CBEngine/vendor/`):
- Assimp - 3D model loading (DLL copied to output post-build)
- stb_image - Image loading (header-only)
- voxelizer - Mesh voxelization

## Profiling

Profiler macros output Chrome DevTools compatible JSON:
- `CB_PROFILE_FUNCTION()` - Profile current function
- `CB_PROFILE_SCOPE("name")` - Profile named scope

ImGui profiler panel toggles with F3 key.

Profile session files are written to the working directory (e.g., `CBEngineProfile-Runtime.json`).

## Creating a New Layer

```cpp
#include <CBEngine.h>

class MyLayer : public CB::Layer
{
public:
    MyLayer() : Layer("MyLayer") {}

    void OnAttach() override
    {
        // Initialize resources
        m_Shader = CB::Shader::Create("assets/shaders/PBR.glsl");
        m_Mesh = CB::Mesh::Load("assets/models/model.fbx");
        m_Material = CB::CreateRef<CB::Material>();
    }

    void OnUpdate(CB::Timestep ts) override
    {
        // Update camera
        m_CameraController.OnUpdate(ts);

        // Render
        CB::RenderCommand::SetClearColor({0.1f, 0.1f, 0.15f, 1.0f});
        CB::RenderCommand::Clear();

        auto& camera = m_CameraController.GetCamera();
        CB::Renderer3D::BeginScene(camera, camera.GetPosition());
        CB::Renderer3D::Submit(m_Shader, m_Material, m_Mesh, transform);
        CB::Renderer3D::EndScene();
    }

    void OnEvent(CB::Event& e) override
    {
        m_CameraController.OnEvent(e);
    }

    void OnImGuiRender() override
    {
        ImGui::Begin("Debug");
        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
        ImGui::End();
    }
};

// In your Application subclass:
PushLayer(new MyLayer());
```

## Shader Format

Shaders use a single-file format with `#type` directives:
```glsl
#type vertex
#version 450 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TexCoords;

uniform mat4 u_ViewProjection;
uniform mat4 u_Transform;

out vec3 v_Normal;

void main()
{
    gl_Position = u_ViewProjection * u_Transform * vec4(a_Position, 1.0);
    v_Normal = mat3(u_Transform) * a_Normal;
}

#type fragment
#version 450 core

layout(location = 0) out vec4 o_Color;

in vec3 v_Normal;

uniform vec3 u_Albedo;

void main()
{
    vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
    float diff = max(dot(normalize(v_Normal), lightDir), 0.0);
    o_Color = vec4(u_Albedo * (0.1 + diff * 0.9), 1.0);
}
```

See `CBEngine-Editor/assets/shaders/PBR.glsl` for a complete PBR shader example.

## Material Format

Materials use a simple key=value text format (`.mat` extension):
```ini
# CBEngine Material File
albedo=1.0,0.5,0.2
metallic=0.0
roughness=0.5
smoothShading=1.0
albedoMap=textures/wood_diffuse.png
normalMap=textures/wood_normal.png
metallicMap=
roughnessMap=
```

**Properties:**
- `albedo` - RGB color (0.0-1.0 each)
- `metallic` - Metallic factor (0.0-1.0)
- `roughness` - Roughness factor (0.0-1.0)
- `smoothShading` - Smooth shading blend (0.0-1.0)
- `albedoMap`, `normalMap`, `metallicMap`, `roughnessMap` - Texture paths (relative to .mat file)

**Usage:**
```cpp
// Load from file
auto material = CB::Material::Load("assets/materials/wood.mat");

// Or via AssetManager
auto material = CB::AssetManager::GetAsset<CB::Material>("materials/wood.mat");

// Create programmatically
auto mat = CB::Material::Create();
mat->SetAlbedo({1.0f, 0.8f, 0.6f});
mat->SetRoughness(0.7f);
mat->SetAlbedoMap(texture, "textures/brick.png");  // Path for serialization
mat->Save("assets/materials/brick.mat");
```

## Include Paths

When including engine headers in client code:
```cpp
#include <CBEngine.h>            // Main header (includes everything)

// Or individual headers:
#include "CBEngine/Core/Application.h"
#include "CBEngine/Core/Layer.h"
#include "CBEngine/Input/Input.h"
#include "CBEngine/Renderer/Core/Renderer3D.h"
#include "CBEngine/Renderer/Resources/Mesh.h"
#include "CBEngine/Utils/VoxelizerAPI.h"
#include "CBEngine/Scene/Scene.h"
#include "CBEngine/Scene/Entity.h"
#include "CBEngine/Components/Components.h"
```
