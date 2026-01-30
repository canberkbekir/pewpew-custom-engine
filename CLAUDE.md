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

- **PewPew/** - Core engine static library
- **Sandbox/** - Test application that links PewPew

### Core Systems

**Application Lifecycle** (`PewPew/src/PewPew/Application.h`):
- `Application::Run()` is the main loop: process events → update layers → render ImGui → swap buffers
- Applications are created via `CreateApplication()` defined by the client (Sandbox)
- Entry point is in `EntryPoint.h` which defines `main()`

**Layer System** (`PewPew/src/PewPew/Layer.h`, `LayerStack.h`):
- Game logic is organized into Layers with `OnUpdate()`, `OnEvent()`, `OnImGuiRender()` methods
- Layers are pushed to the Application's LayerStack

**Event System** (`PewPew/src/PewPew/Events/`):
- Events propagate through layers via `OnEvent()` callback
- Event types: KeyEvent, MouseEvent, ApplicationEvent, WindowCloseEvent
- Uses EventDispatcher pattern for type-safe event handling

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

### Memory Management

Uses custom smart pointer aliases defined in `Core.h`:
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

## Profiling

Profiler macros output Chrome DevTools compatible JSON:
- `PEW_PROFILE_FUNCTION()` - Profile current function
- `PEW_PROFILE_SCOPE("name")` - Profile named scope

ImGui profiler panel toggles with F3 key.

Profile session files are written to Sandbox directory (e.g., `PewPewProfile-Runtime.json`).

## Creating a New Layer

```cpp
class MyLayer : public PewPew::Layer
{
public:
    MyLayer() : Layer("MyLayer") {}

    void OnUpdate(PewPew::TimeStep ts) override { /* per-frame logic */ }
    void OnEvent(PewPew::Event& e) override { /* handle events */ }
    void OnImGuiRender() override { /* debug UI */ }
};

// In your Application subclass:
PushLayer(new MyLayer());
```

## Shader Format

Shaders use a single-file format with `#type` directives:
```glsl
#type vertex
// vertex shader code

#type fragment
// fragment shader code
```

See `Sandbox/assets/shaders/PBR.glsl` for a complete PBR shader example.
