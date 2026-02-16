<p align="center">
  <img src="CBEngine-Editor/resources/icon.ico" alt="CBEngine Logo" width="120"/>
</p>

<h1 align="center">CBEngine</h1>

<p align="center">
  <b>A custom 3D game engine built from scratch in C++17 with a focus on voxel-based workflows</b>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/C%2B%2B-17-blue?style=for-the-badge&logo=cplusplus&logoColor=white" alt="C++17"/>
  <img src="https://img.shields.io/badge/OpenGL-4.5-green?style=for-the-badge&logo=opengl&logoColor=white" alt="OpenGL"/>
  <img src="https://img.shields.io/badge/Platform-Windows-lightgrey?style=for-the-badge&logo=windows&logoColor=white" alt="Windows"/>
  <img src="https://img.shields.io/badge/Build-Premake5-orange?style=for-the-badge" alt="Premake5"/>
</p>

---

<!-- TODO: Add a hero screenshot or GIF of the editor here -->
<!-- ![CBEngine Editor](screenshots/editor-overview.png) -->

## About

CBEngine is a **custom-built 3D game engine** developed entirely from scratch in **C++17** using **OpenGL**. It features a full editor with docking UI, PBR rendering, an entity-component system, and a unique **mesh-to-voxel pipeline** with per-voxel material painting. The engine is designed for creating voxel-style games with destructible environments.

---

## Features

### PBR Rendering

Physically-Based Rendering using Cook-Torrance BRDF with support for albedo, normal, metallic, and roughness texture maps. Includes ACES tonemapping, directional lighting, and smooth/flat shading control.

<!-- TODO: Add screenshot or video of PBR rendering -->
<!-- ![PBR Rendering](screenshots/pbr-rendering.png) -->

---

### Mesh-to-Voxel Conversion

Automatically convert any 3D model (FBX, OBJ, GLTF) into a voxel representation. Supports solid and surface voxelization modes, with texture color sampling that maps the original model's textures onto each voxel.

<!-- TODO: Add video showing mesh-to-voxel conversion -->
<!-- ![Mesh to Voxel](screenshots/mesh-to-voxel.gif) -->

---

### Voxel Texture Editor

A dedicated editor for painting per-voxel materials. Features a **2D slice view** for layer-by-layer editing (Dwarf Fortress style), custom brush creation, a color palette system with 256 entries, and real-time mesh rebuilding as you paint.

<!-- TODO: Add video of voxel texture editor in action -->
<!-- ![Voxel Texture Editor](screenshots/voxel-texture-editor.gif) -->

---

### Mesh Import Pipeline

A full import wizard for 3D models with live preview, voxelization settings, material slot editing, and async voxelization preview. Outputs to processed `.mesh` or voxelized `.vmesh` formats.

<!-- TODO: Add screenshot of mesh import panel -->
<!-- ![Mesh Import](screenshots/mesh-import.png) -->

---

### Custom Editor

A full-featured ImGui-based editor with docking layout, including:

- **Scene Hierarchy** - Entity tree with parent/child relationships and drag-and-drop reparenting
- **Properties Panel** - Component editing with custom widgets per component type
- **Content Browser** - File browser with thumbnails, search, filters, sorting, and drag-and-drop import
- **Viewport** - 3D scene view with entity picking (click to select) and transform gizmos
- **Console** - Log output with level filtering
- **Profiler** - Real-time FPS graph and per-scope profiling

<!-- TODO: Add screenshot of the full editor layout -->
<!-- ![Editor](screenshots/editor-layout.png) -->

---

### Entity Component System

Built on **EnTT** with a full entity hierarchy system including parent/child transforms with world-space caching. Comes with built-in components:

| Component | Description |
|---|---|
| **TransformComponent** | Position, rotation, scale with parent/child hierarchy |
| **MeshRendererComponent** | Mesh + material + shader rendering |
| **VoxelRendererComponent** | Voxel mesh rendering with palette support |
| **DirectionalLightComponent** | Directional lighting with intensity control |

---

### Palette-Based Voxel Materials

A 256-entry color palette system where each entry stores color, metallic, roughness, and emission values. Includes automatic color quantization, palette texture generation, and material type classification (Stone, Wood, Metal, Glass, Marble).

<!-- TODO: Add screenshot of palette system -->
<!-- ![Palette System](screenshots/palette-system.png) -->

---

### Asset Pipeline

A complete asset management system with:

- **UUID-based tracking** with `.meta` files for every asset
- **Hot-reloading** - Shaders, materials, textures, and meshes auto-reload on file changes
- **File watcher** - Automatic change detection using native Windows APIs
- **Custom binary formats** - `.mesh` (processed meshes), `.vmesh` (voxel meshes), `.vtex` (voxel textures)
- **YAML scene serialization** - Human-readable `.scene` files

---

### Scene Serialization

Full scene save/load with YAML-based `.scene` files. All entities, components, hierarchies, and asset references are serialized and restored.

<!-- TODO: Add video of saving and loading a scene -->

---

### Shader System

Single-file GLSL shader format using `#type` directives to define vertex and fragment shaders in one file. Supports automatic compilation, uniform management, and hot-reloading.

```glsl
#type vertex
// vertex shader code...

#type fragment
// fragment shader code...
```

---

### Material System

PBR material files (`.mat`) with albedo, metallic, roughness, smooth shading, and texture map slots. Materials can be created programmatically or loaded from files, and support hot-reloading.

<!-- TODO: Add screenshot of material editing -->
<!-- ![Material Editor](screenshots/material-editor.png) -->

---

### Profiling & Debugging

Chrome DevTools compatible profiling with per-function and per-scope macros. Includes a real-time ImGui profiler panel with FPS history graph (120 frames), toggled with **F3**.

<!-- TODO: Add screenshot of profiler panel -->
<!-- ![Profiler](screenshots/profiler.png) -->

---

## Tech Stack

| Category | Technology |
|---|---|
| **Language** | C++17 |
| **Graphics** | OpenGL 4.5 |
| **Windowing** | GLFW |
| **GL Loader** | Glad |
| **UI** | Dear ImGui (Docking) |
| **Math** | GLM |
| **Logging** | spdlog |
| **3D Loading** | Assimp (FBX, OBJ, GLTF, GLB) |
| **Image Loading** | stb_image (PNG, JPG, TGA, BMP) |
| **ECS** | EnTT |
| **Build System** | Premake5 / Visual Studio 2022 |

---

## Roadmap

- [ ] Voxel physics with custom collision system
- [ ] Destructible voxel environments
- [ ] Fragment spawning on impact
- [ ] Point & spot lights
- [ ] Camera component for game view
- [ ] Shadow mapping

---

## Building

**Requirements:** Visual Studio 2022, Windows 10/11

```bash
# Generate project files
./GenerateProject.bat

# Build via MSBuild
msbuild PewPew.sln /p:Configuration=Debug /p:Platform=x64
```

Or open `PewPew.sln` in Visual Studio and build from there.

---

<p align="center">
  Built from scratch with C++ and OpenGL
</p>
