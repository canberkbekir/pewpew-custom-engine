# PewPew Engine Documentation

A C++17 Windows game engine with PBR rendering, built using Premake5 and Visual Studio 2022.

## Documentation Index

| Document | Description |
|----------|-------------|
| [Architecture Overview](./ARCHITECTURE.md) | Engine architecture, system diagrams, and design patterns |
| [Getting Started](./GETTING_STARTED.md) | Build instructions, project setup, and first steps |
| [Core Systems](./CORE_SYSTEMS.md) | Application lifecycle, layers, events, input, and windowing |
| [Renderer Guide](./RENDERER.md) | 3D rendering, shaders, materials, meshes, and cameras |
| [Voxelizer API](./VoxelizerAPI.md) | Mesh-to-voxel conversion system |
| [API Reference](./API_REFERENCE.md) | Quick reference for all public APIs |

## Quick Start

```batch
:: Generate Visual Studio 2022 solution
GenerateProject.bat

:: Build from command line
msbuild PewPew.sln /p:Configuration=Debug /p:Platform=x64
```

## Project Structure

```
PewPew/                     # Core engine (static library)
├── src/
│   ├── PewPew/            # Engine source code
│   │   ├── Core/          # Core utilities (TimeStep, assertions)
│   │   ├── Events/        # Event system
│   │   ├── Renderer/      # Rendering system
│   │   │   ├── Core/      # Renderer, RendererAPI, RenderCommand
│   │   │   ├── Camera/    # Camera implementations
│   │   │   └── Resources/ # Shader, Texture, Mesh, Material
│   │   ├── ImGui/         # Debug UI integration
│   │   ├── Debug/         # Profiling system
│   │   └── Math/          # GLM type aliases
│   └── Platform/          # Platform-specific code
│       ├── Windows/       # GLFW window, input
│       └── OpenGL/        # OpenGL renderer
└── vendor/                # Third-party dependencies

Sandbox/                   # Test application
├── src/                   # Application code
└── assets/               # Models, textures, shaders
```

## Features

- **PBR Rendering** - Physically-based rendering with metallic-roughness workflow
- **Model Loading** - FBX, OBJ, GLTF support via Assimp
- **Layer System** - Modular game logic organization
- **Event System** - Type-safe event handling with dispatcher pattern
- **Camera Controllers** - First-person camera with WASD + mouse controls
- **ImGui Integration** - Built-in debug UI panels
- **Profiling** - Chrome DevTools compatible profiling output
- **Voxelization** - Convert meshes to voxel representations

## Dependencies

| Library | Purpose |
|---------|---------|
| GLFW | Window management and input |
| Glad | OpenGL function loader |
| ImGui | Immediate mode debug UI |
| glm | Mathematics library |
| spdlog | Logging framework |
| Assimp | 3D model loading |
| stb_image | Image loading |

## License

See the LICENSE file for details.
