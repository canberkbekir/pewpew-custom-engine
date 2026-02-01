# Architecture Overview

This document describes the high-level architecture of the PewPew engine, including system diagrams and design patterns.

## Table of Contents

- [System Architecture](#system-architecture)
- [Application Lifecycle](#application-lifecycle)
- [Renderer Architecture](#renderer-architecture)
- [Event Flow](#event-flow)
- [Layer System](#layer-system)
- [Memory Management](#memory-management)
- [Design Patterns](#design-patterns)

---

## System Architecture

The engine follows a layered architecture with clear separation between core systems, platform abstraction, and client applications.

```mermaid
graph TB
    subgraph "Client Application (Sandbox)"
        APP[Application Subclass]
        LAYERS[Custom Layers]
    end

    subgraph "PewPew Engine"
        subgraph "Core Systems"
            APPLICATION[Application]
            LAYERSTACK[LayerStack]
            EVENTS[Event System]
            INPUT[Input System]
            LOG[Logging]
        end

        subgraph "Renderer"
            RENDERER3D[Renderer3D]
            RENDERER[Renderer]
            RENDERAPI[RendererAPI]
            RENDERCMD[RenderCommand]
        end

        subgraph "Resources"
            SHADER[Shader]
            TEXTURE[Texture]
            MESH[Mesh]
            MATERIAL[Material]
            CAMERA[Camera]
        end

        subgraph "Debug"
            IMGUI[ImGui Layer]
            PROFILER[Instrumentor]
        end
    end

    subgraph "Platform Layer"
        WINDOW[WindowsWindow]
        WININPUT[WindowsInput]
        OGLRENDER[OpenGLRendererAPI]
        OGLSHADER[OpenGLShader]
        OGLTEX[OpenGLTexture]
    end

    subgraph "External Libraries"
        GLFW[GLFW]
        GLAD[Glad]
        GLM[glm]
        ASSIMP[Assimp]
        STB[stb_image]
        SPDLOG[spdlog]
    end

    APP --> APPLICATION
    LAYERS --> LAYERSTACK
    APPLICATION --> LAYERSTACK
    APPLICATION --> EVENTS
    APPLICATION --> RENDERER3D
    APPLICATION --> IMGUI

    RENDERER3D --> RENDERCMD
    RENDERER --> RENDERCMD
    RENDERCMD --> RENDERAPI

    RENDERAPI -.-> OGLRENDER
    SHADER -.-> OGLSHADER
    TEXTURE -.-> OGLTEX

    WINDOW --> GLFW
    OGLRENDER --> GLAD
    MESH --> ASSIMP
    TEXTURE --> STB
    LOG --> SPDLOG
    CAMERA --> GLM
```

### Component Responsibilities

| Component | Responsibility |
|-----------|----------------|
| **Application** | Main loop, window management, layer orchestration |
| **LayerStack** | Ordered collection of game logic layers |
| **Event System** | Type-safe event creation and dispatching |
| **Renderer3D** | 3D scene rendering with PBR lighting |
| **RendererAPI** | Graphics API abstraction (OpenGL) |
| **Resources** | GPU resource management (shaders, textures, meshes) |

---

## Application Lifecycle

The application follows a standard game loop pattern with initialization, update, render, and shutdown phases.

```mermaid
flowchart TD
    subgraph "Startup Phase"
        MAIN[main] --> LOGINIT[Log::Init]
        LOGINIT --> CREATEAPP[CreateApplication]
        CREATEAPP --> APPCTOR[Application Constructor]
        APPCTOR --> WINDOW_CREATE[Create Window]
        WINDOW_CREATE --> RENDERER_INIT[Renderer::Init]
        RENDERER_INIT --> RENDERER3D_INIT[Renderer3D::Init]
        RENDERER3D_INIT --> IMGUI_INIT[ImGuiLayer::OnAttach]
    end

    subgraph "Main Loop"
        IMGUI_INIT --> RUN[Application::Run]
        RUN --> LOOP_START{m_Running?}

        LOOP_START -->|Yes| TIMESTEP[Calculate Timestep]
        TIMESTEP --> CHECK_MIN{Minimized?}

        CHECK_MIN -->|No| UPDATE_LAYERS[Update All Layers]
        CHECK_MIN -->|Yes| IMGUI_BEGIN

        UPDATE_LAYERS --> IMGUI_BEGIN[ImGuiLayer::Begin]
        IMGUI_BEGIN --> IMGUI_RENDER[Layers::OnImGuiRender]
        IMGUI_RENDER --> IMGUI_END[ImGuiLayer::End]
        IMGUI_END --> SWAP[Window::OnUpdate/SwapBuffers]
        SWAP --> LOOP_START

        LOOP_START -->|No| SHUTDOWN
    end

    subgraph "Shutdown Phase"
        SHUTDOWN[Shutdown] --> RENDERER3D_SHUT[Renderer3D::Shutdown]
        RENDERER3D_SHUT --> RENDERER_SHUT[Renderer::Shutdown]
        RENDERER_SHUT --> DELETE_APP[delete app]
    end
```

### Frame Timeline

```mermaid
sequenceDiagram
    participant App as Application
    participant Stack as LayerStack
    participant Layer as Game Layer
    participant ImGui as ImGuiLayer
    participant Window as Window

    loop Every Frame
        App->>App: Calculate deltaTime

        alt Not Minimized
            App->>Stack: Iterate Layers
            Stack->>Layer: OnUpdate(timestep)
            Layer->>Layer: Game Logic
            Layer->>Layer: Render Scene
        end

        App->>ImGui: Begin()
        App->>Stack: Iterate Layers
        Stack->>Layer: OnImGuiRender()
        Layer->>Layer: Draw Debug UI
        App->>ImGui: End()

        App->>Window: OnUpdate()
        Window->>Window: Poll Events
        Window->>Window: Swap Buffers
    end
```

---

## Renderer Architecture

The renderer uses a multi-layer abstraction to separate high-level rendering concepts from low-level graphics API calls.

```mermaid
graph TB
    subgraph "High-Level API"
        R3D[Renderer3D]
        R2D[Renderer]
    end

    subgraph "Command Layer"
        RC[RenderCommand]
    end

    subgraph "Abstraction Layer"
        RAPI[RendererAPI]
    end

    subgraph "Platform Implementation"
        OGLAPI[OpenGLRendererAPI]
    end

    subgraph "OpenGL Calls"
        GL[glDrawElements, glClear, etc.]
    end

    R3D --> RC
    R2D --> RC
    RC --> RAPI
    RAPI -.->|Virtual| OGLAPI
    OGLAPI --> GL
```

### Render Pipeline Flow

```mermaid
flowchart LR
    subgraph "Scene Setup"
        A[BeginScene] --> B[Set Camera]
        B --> C[Set Lighting]
    end

    subgraph "Draw Calls"
        C --> D[Submit Mesh 1]
        D --> E[Submit Mesh 2]
        E --> F[Submit Mesh N]
    end

    subgraph "Frame End"
        F --> G[EndScene]
    end

    subgraph "Per-Submit"
        D -.-> D1[Bind Shader]
        D1 -.-> D2[Bind Material/Textures]
        D2 -.-> D3[Set Uniforms]
        D3 -.-> D4[Bind VertexArray]
        D4 -.-> D5[DrawIndexed]
    end
```

### Resource Ownership

```mermaid
graph TD
    subgraph "Shared Resources (Ref)"
        SHADER[Shader]
        TEXTURE[Texture2D]
        MESH[Mesh]
        MATERIAL[Material]
        VB[VertexBuffer]
        IB[IndexBuffer]
        VA[VertexArray]
    end

    subgraph "Unique Resources (Scope)"
        WINDOW[Window]
        CONTEXT[GraphicsContext]
        RAPI[RendererAPI]
    end

    MESH --> VA
    VA --> VB
    VA --> IB
    MATERIAL --> TEXTURE
```

---

## Event Flow

Events propagate from the window through the application to individual layers.

```mermaid
sequenceDiagram
    participant GLFW
    participant Window as WindowsWindow
    participant App as Application
    participant Stack as LayerStack
    participant Overlay as ImGuiLayer
    participant Layer as Game Layer

    GLFW->>Window: GLFW Callback
    Window->>Window: Create PewPew Event
    Window->>App: EventCallback(event)

    App->>App: OnEvent(event)

    alt Window Event
        App->>App: Dispatch WindowClose
        App->>App: Dispatch WindowResize
    end

    Note over App,Layer: Events propagate in REVERSE order (overlays first)

    App->>Stack: Reverse Iterate
    Stack->>Overlay: OnEvent(event)

    alt Event Not Handled
        Stack->>Layer: OnEvent(event)
        Layer->>Layer: EventDispatcher
        Layer->>Layer: Handle Specific Events
    end
```

### Event Types Hierarchy

```mermaid
classDiagram
    class Event {
        +bool Handled
        +GetEventType()
        +GetName()
        +GetCategoryFlags()
        +IsInCategory()
    }

    class KeyEvent {
        #int m_KeyCode
        +GetKeyCode()
    }

    class MouseButtonEvent {
        #int m_Button
        +GetMouseButton()
    }

    Event <|-- KeyEvent
    Event <|-- MouseMovedEvent
    Event <|-- MouseScrolledEvent
    Event <|-- MouseButtonEvent
    Event <|-- WindowResizeEvent
    Event <|-- WindowCloseEvent

    KeyEvent <|-- KeyPressedEvent
    KeyEvent <|-- KeyReleasedEvent
    KeyEvent <|-- KeyTypedEvent

    MouseButtonEvent <|-- MouseButtonPressedEvent
    MouseButtonEvent <|-- MouseButtonReleasedEvent

    class KeyPressedEvent {
        +int m_RepeatCount
        +GetRepeatCount()
    }

    class MouseMovedEvent {
        +float m_MouseX
        +float m_MouseY
        +GetX()
        +GetY()
    }

    class MouseScrolledEvent {
        +float m_XOffset
        +float m_YOffset
    }

    class WindowResizeEvent {
        +unsigned int m_Width
        +unsigned int m_Height
    }
```

---

## Layer System

Layers organize game logic into modular, stackable components.

```mermaid
graph TB
    subgraph "LayerStack (Bottom to Top)"
        direction TB
        L1[Layer 1: World]
        L2[Layer 2: Player]
        L3[Layer 3: UI]
        O1[Overlay: ImGui]
        O2[Overlay: Debug]
    end

    subgraph "Update Order"
        direction LR
        U1[Update L1] --> U2[Update L2]
        U2 --> U3[Update L3]
        U3 --> U4[Update O1]
        U4 --> U5[Update O2]
    end

    subgraph "Event Order (Reverse)"
        direction RL
        E5[Event O2] --> E4[Event O1]
        E4 --> E3[Event L3]
        E3 --> E2[Event L2]
        E2 --> E1[Event L1]
    end

    L1 ~~~ U1
    O2 ~~~ E5
```

### Layer Lifecycle

```mermaid
stateDiagram-v2
    [*] --> Created: new Layer()
    Created --> Attached: PushLayer/PushOverlay
    Attached --> Running: OnAttach()

    state Running {
        [*] --> Update
        Update --> Render: OnUpdate(ts)
        Render --> ImGui: OnImGuiRender()
        ImGui --> Events: OnEvent(e)
        Events --> Update
    }

    Running --> Detached: PopLayer/PopOverlay
    Detached --> [*]: OnDetach()
```

---

## Memory Management

The engine uses two smart pointer types for resource management.

```mermaid
graph LR
    subgraph "Scope (unique_ptr)"
        S1[Single Owner]
        S2[Automatic Cleanup]
        S3[Move-Only]
    end

    subgraph "Ref (shared_ptr)"
        R1[Multiple Owners]
        R2[Reference Counted]
        R3[Copy & Move]
    end

    subgraph "Use Cases"
        UC_SCOPE[Window, RendererAPI]
        UC_REF[Shader, Mesh, Texture, Material]
    end

    S1 --> UC_SCOPE
    R1 --> UC_REF
```

### Factory Pattern Usage

```cpp
// Scope - Single ownership, deterministic cleanup
Scope<Window> window = Window::Create(props);

// Ref - Shared ownership, reference counted
Ref<Shader> shader = Shader::Create("path/to/shader.glsl");
Ref<Mesh> mesh = Mesh::Load("path/to/model.fbx");
Ref<Texture2D> texture = Texture2D::Create("path/to/image.png");
```

---

## Design Patterns

### Patterns Used

| Pattern | Usage | Example |
|---------|-------|---------|
| **Singleton** | Global access to core systems | `Application::Get()`, `Log::GetCoreLogger()` |
| **Factory** | Platform-agnostic resource creation | `Shader::Create()`, `Window::Create()` |
| **Strategy** | Swappable implementations | `RendererAPI` → `OpenGLRendererAPI` |
| **Observer** | Event system | `EventDispatcher`, `EventCallback` |
| **RAII** | Resource cleanup | `InstrumentationTimer`, smart pointers |
| **Command** | Deferred rendering | `RenderCommand` static methods |
| **Facade** | Simplified high-level API | `Renderer3D::Submit()` |

### Class Relationships

```mermaid
classDiagram
    class Application {
        -Scope~Window~ m_Window
        -LayerStack m_LayerStack
        -ImGuiLayer* m_ImGuiLayer
        +Run()
        +OnEvent(Event&)
        +PushLayer(Layer*)
        +static Get() Application&
    }

    class Window {
        <<abstract>>
        +OnUpdate()
        +GetWidth()
        +GetHeight()
        +SetEventCallback()
        +static Create()
    }

    class Layer {
        <<abstract>>
        #String m_DebugName
        +OnAttach()
        +OnDetach()
        +OnUpdate(Timestep)
        +OnEvent(Event&)
        +OnImGuiRender()
    }

    class LayerStack {
        -vector~Layer*~ m_Layers
        -unsigned int m_LayerInsertIndex
        +PushLayer(Layer*)
        +PushOverlay(Layer*)
        +PopLayer(Layer*)
    }

    class RendererAPI {
        <<abstract>>
        +Init()
        +SetViewport()
        +Clear()
        +DrawIndexed()
        +static Create()
    }

    Application "1" *-- "1" Window
    Application "1" *-- "1" LayerStack
    LayerStack "1" o-- "*" Layer
    Window <|-- WindowsWindow
    RendererAPI <|-- OpenGLRendererAPI
```

---

## Platform Abstraction

```mermaid
graph TB
    subgraph "Abstract Interfaces"
        WINDOW[Window]
        INPUT[Input]
        RAPI[RendererAPI]
        SHADER[Shader]
        TEXTURE[Texture]
        BUFFER[Buffer]
    end

    subgraph "Windows/OpenGL Implementation"
        WIN_WINDOW[WindowsWindow]
        WIN_INPUT[WindowsInput]
        OGL_RAPI[OpenGLRendererAPI]
        OGL_SHADER[OpenGLShader]
        OGL_TEX[OpenGLTexture]
        OGL_BUF[OpenGLBuffer]
    end

    WINDOW -.-> WIN_WINDOW
    INPUT -.-> WIN_INPUT
    RAPI -.-> OGL_RAPI
    SHADER -.-> OGL_SHADER
    TEXTURE -.-> OGL_TEX
    BUFFER -.-> OGL_BUF

    subgraph "Future Platforms"
        DX_RAPI[DirectXRendererAPI]
        VK_RAPI[VulkanRendererAPI]
        LINUX[LinuxWindow]
    end

    RAPI -.->|Future| DX_RAPI
    RAPI -.->|Future| VK_RAPI
    WINDOW -.->|Future| LINUX
```

---

## Next Steps

- [Getting Started](./GETTING_STARTED.md) - Set up your development environment
- [Core Systems](./CORE_SYSTEMS.md) - Deep dive into engine systems
- [Renderer Guide](./RENDERER.md) - Learn the rendering pipeline
