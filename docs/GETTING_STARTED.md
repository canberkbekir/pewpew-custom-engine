# Getting Started

This guide will help you set up the development environment, build the engine, and create your first application.

## Table of Contents

- [Prerequisites](#prerequisites)
- [Building the Engine](#building-the-engine)
- [Project Configuration](#project-configuration)
- [Creating Your First Application](#creating-your-first-application)
- [Running the Sandbox](#running-the-sandbox)
- [Troubleshooting](#troubleshooting)

---

## Prerequisites

### Required Software

| Software | Version | Purpose |
|----------|---------|---------|
| **Visual Studio 2022** | 17.0+ | C++ compiler and IDE |
| **Windows SDK** | 10.0+ | Windows development |
| **Git** | 2.0+ | Version control and submodules |

### Visual Studio Workloads

Install these workloads via Visual Studio Installer:
- **Desktop development with C++**
- **Windows 10/11 SDK** (any recent version)

### Cloning the Repository

```batch
git clone --recursive https://github.com/your-repo/pewpew-custom-engine.git
cd pewpew-custom-engine
```

> **Important:** Use `--recursive` to clone all Git submodules (GLFW, Glad, ImGui, etc.)

If you already cloned without `--recursive`:
```batch
git submodule update --init --recursive
```

---

## Building the Engine

### Step 1: Generate Project Files

Run the batch file to generate Visual Studio solution:

```batch
GenerateProject.bat
```

This executes:
```batch
vendor\bin\premake\premake5.exe vs2022
```

Generated files:
- `PewPew.sln` - Visual Studio solution
- `PewPew/PewPew.vcxproj` - Engine project
- `Sandbox/Sandbox.vcxproj` - Test application

### Step 2: Build via Visual Studio

1. Open `PewPew.sln` in Visual Studio 2022
2. Select configuration: **Debug** | **x64**
3. Build → Build Solution (Ctrl+Shift+B)

### Step 2 (Alternative): Build via Command Line

```batch
:: Debug build
msbuild PewPew.sln /p:Configuration=Debug /p:Platform=x64

:: Release build
msbuild PewPew.sln /p:Configuration=Release /p:Platform=x64

:: Distribution build (no debug symbols)
msbuild PewPew.sln /p:Configuration=Dist /p:Platform=x64
```

### Build Configurations

| Configuration | Purpose | Defines |
|---------------|---------|---------|
| **Debug** | Development with full debugging | `PEW_DEBUG` |
| **Release** | Optimized with debug info | `PEW_RELEASE` |
| **Dist** | Distribution, no debug overhead | `PEW_DIST` |

### Output Directories

```
bin/Debug-windows-x64/Sandbox/Sandbox.exe
bin/Debug-windows-x64/PewPew/PewPew.lib
bin/Release-windows-x64/...
bin/Dist-windows-x64/...
```

---

## Project Configuration

### Premake Build System

The project uses [Premake5](https://premake.github.io/) to generate IDE-specific project files.

**Key files:**
- `premake5.lua` - Main build configuration
- `PewPew/vendor/*/premake5.lua` - Dependency configurations

### Adding New Source Files

1. Add `.cpp`/`.h` files to appropriate directory
2. Re-run `GenerateProject.bat`
3. Reload solution in Visual Studio

### Changing Build Settings

Edit `premake5.lua`:

```lua
-- Example: Add a new define
defines {
    "PEW_PLATFORM_WINDOWS",
    "MY_CUSTOM_DEFINE"  -- Add here
}

-- Example: Link additional library
links {
    "PewPew",
    "MyLibrary"  -- Add here
}
```

Then regenerate: `GenerateProject.bat`

---

## Creating Your First Application

### Step 1: Create Application Class

```cpp
// MyApp.cpp
#include <PewPew.h>

class MyApp : public PewPew::Application
{
public:
    MyApp()
    {
        // Push your layers here
        PushLayer(new MyGameLayer());
    }

    ~MyApp()
    {
    }
};

// Entry point - required by the engine
PewPew::Application* PewPew::CreateApplication()
{
    return new MyApp();
}
```

### Step 2: Create a Game Layer

```cpp
// MyGameLayer.h
#pragma once
#include <PewPew.h>

class MyGameLayer : public PewPew::Layer
{
public:
    MyGameLayer()
        : Layer("MyGameLayer")
    {
    }

    void OnAttach() override
    {
        // Initialize resources
        m_Shader = PewPew::Shader::Create("assets/shaders/Basic.glsl");
        m_Mesh = PewPew::Mesh::Load("assets/models/Cube.obj");

        m_Material = PewPew::CreateRef<PewPew::Material>();
        m_Material->SetAlbedo({0.8f, 0.2f, 0.2f});  // Red color
        m_Material->SetRoughness(0.5f);
        m_Material->SetMetallic(0.0f);

        // Setup camera
        float aspectRatio = 1280.0f / 720.0f;
        m_CameraController = PewPew::CreateScope<PewPew::PerspectiveCameraController>(
            45.0f, aspectRatio, 0.1f, 1000.0f
        );
    }

    void OnDetach() override
    {
        // Cleanup
    }

    void OnUpdate(PewPew::Timestep ts) override
    {
        // Update camera
        m_CameraController->OnUpdate(ts);

        // Clear screen
        PewPew::RenderCommand::SetClearColor({0.1f, 0.1f, 0.15f, 1.0f});
        PewPew::RenderCommand::Clear();

        // Render scene
        auto& camera = m_CameraController->GetCamera();
        PewPew::Renderer3D::BeginScene(camera, camera.GetPosition());

        glm::mat4 transform = glm::translate(glm::mat4(1.0f), {0.0f, 0.0f, -5.0f});
        PewPew::Renderer3D::Submit(m_Shader, m_Material, m_Mesh, transform);

        PewPew::Renderer3D::EndScene();
    }

    void OnImGuiRender() override
    {
        // Debug UI
        ImGui::Begin("Debug");
        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
        ImGui::ColorEdit3("Albedo", &m_Material->GetAlbedo().x);
        ImGui::SliderFloat("Roughness", &m_Roughness, 0.0f, 1.0f);
        if (ImGui::SliderFloat("Roughness", &m_Roughness, 0.0f, 1.0f))
            m_Material->SetRoughness(m_Roughness);
        ImGui::End();
    }

    void OnEvent(PewPew::Event& e) override
    {
        m_CameraController->OnEvent(e);

        PewPew::EventDispatcher dispatcher(e);
        dispatcher.Dispatch<PewPew::KeyPressedEvent>(
            [this](PewPew::KeyPressedEvent& event) {
                if (event.GetKeyCode() == PEW_KEY_ESCAPE)
                    PewPew::Application::Get().Close();
                return false;
            });
    }

private:
    PewPew::Ref<PewPew::Shader> m_Shader;
    PewPew::Ref<PewPew::Mesh> m_Mesh;
    PewPew::Ref<PewPew::Material> m_Material;
    PewPew::Scope<PewPew::PerspectiveCameraController> m_CameraController;
    float m_Roughness = 0.5f;
};
```

### Step 3: Basic Shader

Create `assets/shaders/Basic.glsl`:

```glsl
#type vertex
#version 450 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TexCoords;

uniform mat4 u_ViewProjection;
uniform mat4 u_Transform;

out vec3 v_Position;
out vec3 v_Normal;
out vec2 v_TexCoords;

void main()
{
    v_Position = vec3(u_Transform * vec4(a_Position, 1.0));
    v_Normal = mat3(transpose(inverse(u_Transform))) * a_Normal;
    v_TexCoords = a_TexCoords;
    gl_Position = u_ViewProjection * u_Transform * vec4(a_Position, 1.0);
}

#type fragment
#version 450 core

layout(location = 0) out vec4 o_Color;

in vec3 v_Position;
in vec3 v_Normal;
in vec2 v_TexCoords;

uniform vec3 u_Albedo;
uniform vec3 u_CameraPosition;
uniform vec3 u_LightDirection;
uniform vec3 u_LightColor;

void main()
{
    vec3 normal = normalize(v_Normal);
    vec3 lightDir = normalize(-u_LightDirection);

    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * u_LightColor;

    vec3 ambient = vec3(0.1);
    vec3 result = (ambient + diffuse) * u_Albedo;

    o_Color = vec4(result, 1.0);
}
```

---

## Running the Sandbox

The Sandbox project is the built-in test application.

### From Visual Studio

1. Right-click **Sandbox** in Solution Explorer
2. Set as Startup Project
3. Press F5 to run with debugging

### From Command Line

```batch
cd bin\Debug-windows-x64\Sandbox
Sandbox.exe
```

### Default Controls

| Key | Action |
|-----|--------|
| **W/A/S/D** | Move camera |
| **Mouse** | Look around |
| **Scroll** | Zoom (FOV) |
| **F3** | Toggle profiler panel |
| **ESC** | Exit application |

---

## Troubleshooting

### "Cannot find assimp-vc143-mt.dll"

The DLL should be copied automatically during build. If missing:

```batch
copy PewPew\vendor\assimp\lib\assimp-vc143-mt.dll bin\Debug-windows-x64\Sandbox\
```

### "Git submodule not found"

Initialize submodules:
```batch
git submodule update --init --recursive
```

### "Premake5 not found"

Ensure you're running from the repository root:
```batch
cd C:\path\to\pewpew-custom-engine
GenerateProject.bat
```

### "Missing glfw3.lib"

GLFW builds as part of the solution. Build the entire solution first:
```batch
msbuild PewPew.sln /p:Configuration=Debug /p:Platform=x64
```

### Build Errors After Pulling Changes

1. Close Visual Studio
2. Delete generated files:
   ```batch
   del /s /q *.vcxproj *.vcxproj.filters *.sln
   ```
3. Regenerate:
   ```batch
   GenerateProject.bat
   ```
4. Reopen solution

### Shader Compilation Errors

Check the console output for GLSL errors. Common issues:
- Missing `#version` directive
- Incorrect uniform names (case-sensitive)
- Type mismatches in uniforms

---

## Next Steps

- [Core Systems](./CORE_SYSTEMS.md) - Learn about layers, events, and input
- [Renderer Guide](./RENDERER.md) - Deep dive into 3D rendering
- [API Reference](./API_REFERENCE.md) - Complete API documentation
