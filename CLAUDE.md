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
msbuild PewPew.sln /p:Configuration=Debug /p:Platform=x64
msbuild PewPew.sln /p:Configuration=Release /p:Platform=x64
```

**Configurations:** Debug, Release, Dist (distribution)

**Output directories:**
- Binaries: `bin/{Config}-windows-x64/{ProjectName}/`
- Intermediates: `bin-int/{Config}-windows-x64/{ProjectName}/`

## Architecture

### Project Structure

```
PewPew/                          # Core engine (static library)
├── src/
│   ├── pewpch.h/cpp             # Precompiled header
│   ├── PewPew.h                 # Main include header for clients
│   ├── PewPew/                  # Engine source code
│   │   ├── Core/                # Core systems
│   │   │   ├── Application.h/cpp    # Main application class
│   │   │   ├── Core.h               # Macros, smart pointers, assertions
│   │   │   ├── EntryPoint.h         # main() definition
│   │   │   ├── Layer.h/cpp          # Layer base class
│   │   │   ├── LayerStack.h/cpp     # Layer management
│   │   │   ├── Log.h/cpp            # Logging system
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
│   │   │   └── MouseEvent.h         # Mouse events
│   │   │
│   │   ├── Renderer/            # Rendering system
│   │   │   ├── Core/                # Renderer core
│   │   │   │   ├── Renderer.h/cpp       # Basic renderer
│   │   │   │   ├── Renderer3D.h/cpp     # 3D PBR renderer
│   │   │   │   ├── RenderCommand.h/cpp  # Low-level commands
│   │   │   │   ├── RendererAPI.h/cpp    # Graphics API abstraction
│   │   │   │   └── GraphicsContext.h/cpp
│   │   │   ├── Camera/              # Camera implementations
│   │   │   │   ├── Camera.h
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
│   │   ├── Utils/               # Utility systems
│   │   │   └── VoxelizerAPI.h/cpp   # Mesh voxelization
│   │   │
│   │   ├── Debug/               # Debug tools
│   │   │   ├── Instrumentor.h       # Profiling system
│   │   │   └── ProfilerPanel.h/cpp  # ImGui profiler UI
│   │   │
│   │   ├── ImGui/               # ImGui integration
│   │   │   ├── ImGuiLayer.h/cpp
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
│           └── OpenGLVertexArray.h/cpp
│
└── vendor/                      # Third-party dependencies

Sandbox/                         # Test application
├── src/
│   └── SandboxApp.cpp
└── assets/
    ├── models/                  # 3D models (FBX, OBJ, GLTF)
    ├── textures/                # Images (PNG, JPG, TGA)
    └── shaders/                 # GLSL shaders

docs/                            # Documentation
```

### Core Systems

**Application Lifecycle** (`PewPew/src/PewPew/Core/Application.h`):
- `Application::Run()` is the main loop: process events → update layers → render ImGui → swap buffers
- Applications are created via `CreateApplication()` defined by the client (Sandbox)
- Entry point is in `Core/EntryPoint.h` which defines `main()`

**Layer System** (`PewPew/src/PewPew/Core/Layer.h`, `LayerStack.h`):
- Game logic is organized into Layers with `OnUpdate()`, `OnEvent()`, `OnImGuiRender()` methods
- Layers are pushed to the Application's LayerStack

**Event System** (`PewPew/src/PewPew/Events/`):
- Events propagate through layers via `OnEvent()` callback
- Event types: KeyEvent, MouseEvent, ApplicationEvent, WindowCloseEvent
- Uses EventDispatcher pattern for type-safe event handling

**Input System** (`PewPew/src/PewPew/Input/`):
- `Input::IsKeyPressed()`, `Input::IsMouseButtonPressed()` for polling
- Key codes in `KeyCodes.h`, mouse buttons in `MouseButtonCodes.h`

**Renderer** (`PewPew/src/PewPew/Renderer/`):
- `Renderer3D` handles 3D scene rendering with PBR support
- `RendererAPI` abstracts graphics API (OpenGL implementation in `Platform/OpenGL/`)
- `RenderCommand` provides low-level draw calls

**Resources** (`PewPew/src/PewPew/Renderer/Resources/`):
- `Mesh` - 3D model loading via Assimp (FBX, OBJ, GLTF)
- `Shader` - GLSL shader compilation and uniform management
- `Texture` - Image loading via stb_image
- `Material` - PBR material properties (albedo, normal, roughness, metallic)

**Camera** (`PewPew/src/PewPew/Renderer/Camera/`):
- `PerspectiveCamera`, `OrthographicCamera` with view/projection matrices
- `PerspectiveCameraController` for WASD + mouse control

**Utilities** (`PewPew/src/PewPew/Utils/`):
- `VoxelizerAPI` - Convert meshes to voxel representations

### Memory Management

Uses custom smart pointer aliases defined in `Core/Core.h`:
- `Scope<T>` = `std::unique_ptr<T>` (use `CreateScope<T>()`)
- `Ref<T>` = `std::shared_ptr<T>` (use `CreateRef<T>()`)

### Platform Abstraction

- `Platform/Windows/` - GLFW-based window and input implementation
- `Platform/OpenGL/` - OpenGL renderer implementation
- Platform macros: `PEW_PLATFORM_WINDOWS`, `PEW_DEBUG`, `PEW_RELEASE`, `PEW_DIST`

## Dependencies

**Git submodules** (in `PewPew/vendor/`):
- GLFW - Window management
- Glad - OpenGL loader
- ImGui - Debug UI
- glm - Math library
- spdlog - Logging

**Prebuilt** (in `PewPew/vendor/`):
- Assimp - 3D model loading (DLL copied to output post-build)
- stb_image - Image loading (header-only)
- voxelizer - Mesh voxelization

## Profiling

Profiler macros output Chrome DevTools compatible JSON:
- `PEW_PROFILE_FUNCTION()` - Profile current function
- `PEW_PROFILE_SCOPE("name")` - Profile named scope

ImGui profiler panel toggles with F3 key.

Profile session files are written to Sandbox directory (e.g., `PewPewProfile-Runtime.json`).

## Creating a New Layer

```cpp
#include <PewPew.h>

class MyLayer : public PewPew::Layer
{
public:
    MyLayer() : Layer("MyLayer") {}

    void OnAttach() override
    {
        // Initialize resources
        m_Shader = PewPew::Shader::Create("assets/shaders/PBR.glsl");
        m_Mesh = PewPew::Mesh::Load("assets/models/model.fbx");
        m_Material = PewPew::CreateRef<PewPew::Material>();
    }

    void OnUpdate(PewPew::Timestep ts) override
    {
        // Update camera
        m_CameraController.OnUpdate(ts);

        // Render
        PewPew::RenderCommand::SetClearColor({0.1f, 0.1f, 0.15f, 1.0f});
        PewPew::RenderCommand::Clear();

        auto& camera = m_CameraController.GetCamera();
        PewPew::Renderer3D::BeginScene(camera, camera.GetPosition());
        PewPew::Renderer3D::Submit(m_Shader, m_Material, m_Mesh, transform);
        PewPew::Renderer3D::EndScene();
    }

    void OnEvent(PewPew::Event& e) override
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

See `Sandbox/assets/shaders/PBR.glsl` for a complete PBR shader example.

## Material Format

Materials use a simple key=value text format (`.mat` extension):
```ini
# PewPew Material File
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
auto material = PewPew::Material::Load("assets/materials/wood.mat");

// Or via AssetManager
auto material = PewPew::AssetManager::GetAsset<PewPew::Material>("materials/wood.mat");

// Create programmatically
auto mat = PewPew::Material::Create();
mat->SetAlbedo({1.0f, 0.8f, 0.6f});
mat->SetRoughness(0.7f);
mat->SetAlbedoMap(texture, "textures/brick.png");  // Path for serialization
mat->Save("assets/materials/brick.mat");
```

## Include Paths

When including engine headers in client code:
```cpp
#include <PewPew.h>              // Main header (includes everything)

// Or individual headers:
#include "PewPew/Core/Application.h"
#include "PewPew/Core/Layer.h"
#include "PewPew/Input/Input.h"
#include "PewPew/Renderer/Core/Renderer3D.h"
#include "PewPew/Renderer/Resources/Mesh.h"
#include "PewPew/Utils/VoxelizerAPI.h"
```

## Documentation

Full documentation available in `docs/`:
- `README.md` - Documentation index
- `ARCHITECTURE.md` - System architecture with diagrams
- `GETTING_STARTED.md` - Build and setup guide
- `CORE_SYSTEMS.md` - Application, layers, events, input
- `RENDERER.md` - 3D rendering guide
- `API_REFERENCE.md` - Complete API reference
- `VoxelizerAPI.md` - Voxelization system
