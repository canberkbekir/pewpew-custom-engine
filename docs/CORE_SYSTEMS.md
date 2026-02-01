# Core Systems

This document covers the foundational systems of the PewPew engine: application lifecycle, layers, events, input, and windowing.

## Table of Contents

- [Application](#application)
- [Layer System](#layer-system)
- [Event System](#event-system)
- [Input System](#input-system)
- [Window System](#window-system)
- [Logging](#logging)
- [Profiling](#profiling)
- [Time Management](#time-management)

---

## Application

The `Application` class is the heart of the engine, managing the main loop and coordinating all subsystems.

### Location
`PewPew/src/PewPew/Application.h`

### Class Overview

```cpp
class Application
{
public:
    Application();
    virtual ~Application();

    void Run();                              // Main game loop
    void OnEvent(Event& e);                  // Event handler
    void Close();                            // Request shutdown

    void PushLayer(Layer* layer);            // Add gameplay layer
    void PushOverlay(Layer* overlay);        // Add overlay (debug UI)

    Window& GetWindow();                     // Access window
    static Application& Get();               // Singleton access

private:
    bool OnWindowClose(WindowCloseEvent& e);
    bool OnWindowResize(WindowResizeEvent& e);

    Scope<Window> m_Window;
    ImGuiLayer* m_ImGuiLayer;
    LayerStack m_LayerStack;
    bool m_Running = true;
    bool m_Minimized = false;
    float m_LastFrameTime = 0.0f;

    static Application* s_Instance;
};
```

### Creating an Application

```cpp
// Your application subclass
class MyGame : public PewPew::Application
{
public:
    MyGame()
    {
        PushLayer(new GameLayer());
        PushLayer(new UILayer());
        // Overlays are pushed after layers
    }
};

// Required factory function
PewPew::Application* PewPew::CreateApplication()
{
    return new MyGame();
}
```

### Main Loop

The `Run()` method implements the main game loop:

```cpp
void Application::Run()
{
    while (m_Running)
    {
        // 1. Calculate delta time
        float time = (float)glfwGetTime();
        Timestep timestep = time - m_LastFrameTime;
        m_LastFrameTime = time;

        // 2. Update layers (skip if minimized)
        if (!m_Minimized)
        {
            for (Layer* layer : m_LayerStack)
                layer->OnUpdate(timestep);
        }

        // 3. Render ImGui
        m_ImGuiLayer->Begin();
        for (Layer* layer : m_LayerStack)
            layer->OnImGuiRender();
        m_ImGuiLayer->End();

        // 4. Swap buffers and poll events
        m_Window->OnUpdate();
    }
}
```

---

## Layer System

Layers organize game logic into modular, stackable components.

### Location
- `PewPew/src/PewPew/Layer.h`
- `PewPew/src/PewPew/LayerStack.h`

### Layer Base Class

```cpp
class Layer
{
public:
    Layer(const String& name = "Layer");
    virtual ~Layer();

    virtual void OnAttach() {}                    // Called when added
    virtual void OnDetach() {}                    // Called when removed
    virtual void OnUpdate(Timestep ts) {}         // Per-frame update
    virtual void OnImGuiRender() {}               // Debug UI rendering
    virtual void OnEvent(Event& event) {}         // Event handling

    const String& GetName() const { return m_DebugName; }

protected:
    String m_DebugName;
};
```

### Creating a Layer

```cpp
class GameLayer : public PewPew::Layer
{
public:
    GameLayer() : Layer("GameLayer") {}

    void OnAttach() override
    {
        // Initialize resources, load assets
        m_Texture = PewPew::Texture2D::Create("assets/player.png");
    }

    void OnDetach() override
    {
        // Cleanup if needed
    }

    void OnUpdate(PewPew::Timestep ts) override
    {
        // Game logic
        m_Position += m_Velocity * (float)ts;

        // Rendering
        PewPew::RenderCommand::Clear();
        // ... render scene
    }

    void OnImGuiRender() override
    {
        ImGui::Begin("Game Debug");
        ImGui::Text("Position: %.2f, %.2f", m_Position.x, m_Position.y);
        ImGui::End();
    }

    void OnEvent(PewPew::Event& e) override
    {
        PewPew::EventDispatcher dispatcher(e);
        dispatcher.Dispatch<PewPew::KeyPressedEvent>(
            PEW_BIND_EVENT_FN(GameLayer::OnKeyPressed));
    }

private:
    bool OnKeyPressed(PewPew::KeyPressedEvent& e)
    {
        if (e.GetKeyCode() == PEW_KEY_SPACE)
            Jump();
        return false;  // Don't consume event
    }
};
```

### Layer Stack

The `LayerStack` manages layer ordering:

```cpp
class LayerStack
{
public:
    void PushLayer(Layer* layer);      // Insert at layer position
    void PushOverlay(Layer* overlay);  // Insert at end (always on top)
    void PopLayer(Layer* layer);
    void PopOverlay(Layer* overlay);

    std::vector<Layer*>::iterator begin();
    std::vector<Layer*>::iterator end();
};
```

**Ordering:**
1. Layers are updated first-to-last (bottom to top)
2. Events propagate last-to-first (top to bottom)
3. Overlays always render after layers

```
Stack:    [Layer1] [Layer2] [Layer3] | [Overlay1] [Overlay2]
Update:   ─────────────────────────────────────────────────►
Events:   ◄─────────────────────────────────────────────────
```

---

## Event System

The event system provides type-safe event handling with a dispatcher pattern.

### Location
- `PewPew/src/PewPew/Events/Event.h`
- `PewPew/src/PewPew/Events/KeyEvent.h`
- `PewPew/src/PewPew/Events/MouseEvent.h`
- `PewPew/src/PewPew/Events/ApplicationEvent.h`

### Event Base Class

```cpp
class Event
{
public:
    bool Handled = false;

    virtual EventType GetEventType() const = 0;
    virtual const char* GetName() const = 0;
    virtual int GetCategoryFlags() const = 0;
    virtual std::string ToString() const { return GetName(); }

    bool IsInCategory(EventCategory category)
    {
        return GetCategoryFlags() & category;
    }
};
```

### Event Types

| Event Class | Properties | Description |
|-------------|------------|-------------|
| `KeyPressedEvent` | keycode, repeatCount | Key pressed or held |
| `KeyReleasedEvent` | keycode | Key released |
| `KeyTypedEvent` | keycode | Character input |
| `MouseMovedEvent` | x, y | Mouse position changed |
| `MouseScrolledEvent` | xOffset, yOffset | Mouse wheel scrolled |
| `MouseButtonPressedEvent` | button | Mouse button pressed |
| `MouseButtonReleasedEvent` | button | Mouse button released |
| `WindowResizeEvent` | width, height | Window resized |
| `WindowCloseEvent` | - | Window close requested |

### Event Categories

```cpp
enum class EventCategory
{
    None = 0,
    EventCategoryApplication    = BIT(0),
    EventCategoryInput          = BIT(1),
    EventCategoryKeyboard       = BIT(2),
    EventCategoryMouse          = BIT(3),
    EventCategoryMouseButton    = BIT(4)
};
```

### Event Dispatcher

The `EventDispatcher` provides type-safe event handling:

```cpp
void OnEvent(PewPew::Event& e)
{
    PewPew::EventDispatcher dispatcher(e);

    // Dispatch to specific handler if type matches
    dispatcher.Dispatch<PewPew::KeyPressedEvent>(
        [this](PewPew::KeyPressedEvent& event) {
            PEW_INFO("Key pressed: {0}", event.GetKeyCode());
            return true;  // Mark as handled
        });

    dispatcher.Dispatch<PewPew::MouseMovedEvent>(
        PEW_BIND_EVENT_FN(MyLayer::OnMouseMoved));
}

bool OnMouseMoved(PewPew::MouseMovedEvent& e)
{
    m_MouseX = e.GetX();
    m_MouseY = e.GetY();
    return false;  // Allow propagation
}
```

### Event Binding Macro

```cpp
// PEW_BIND_EVENT_FN creates a lambda that binds member functions
#define PEW_BIND_EVENT_FN(fn) [this](auto&&... args) -> decltype(auto) { \
    return this->fn(std::forward<decltype(args)>(args)...); \
}
```

---

## Input System

The input system provides per-frame state polling for keyboard and mouse.

### Location
- `PewPew/src/PewPew/Input.h`
- `PewPew/src/PewPew/KeyCodes.h`
- `PewPew/src/PewPew/MouseButtonCodes.h`

### Static API

```cpp
class Input
{
public:
    static bool IsKeyPressed(int keycode);
    static bool IsMouseButtonPressed(int button);
    static std::pair<float, float> GetMousePosition();
    static float GetMouseX();
    static float GetMouseY();
};
```

### Usage

```cpp
void OnUpdate(PewPew::Timestep ts)
{
    // Keyboard input
    if (PewPew::Input::IsKeyPressed(PEW_KEY_W))
        m_Position.y += m_Speed * ts;
    if (PewPew::Input::IsKeyPressed(PEW_KEY_S))
        m_Position.y -= m_Speed * ts;
    if (PewPew::Input::IsKeyPressed(PEW_KEY_A))
        m_Position.x -= m_Speed * ts;
    if (PewPew::Input::IsKeyPressed(PEW_KEY_D))
        m_Position.x += m_Speed * ts;

    // Mouse input
    auto [mouseX, mouseY] = PewPew::Input::GetMousePosition();

    if (PewPew::Input::IsMouseButtonPressed(PEW_MOUSE_BUTTON_LEFT))
        Shoot();
}
```

### Key Codes

Common key codes (defined in `KeyCodes.h`):

```cpp
// Letters
PEW_KEY_A through PEW_KEY_Z

// Numbers
PEW_KEY_0 through PEW_KEY_9

// Function keys
PEW_KEY_F1 through PEW_KEY_F12

// Special keys
PEW_KEY_SPACE
PEW_KEY_ESCAPE
PEW_KEY_ENTER
PEW_KEY_TAB
PEW_KEY_BACKSPACE
PEW_KEY_DELETE

// Arrow keys
PEW_KEY_UP, PEW_KEY_DOWN, PEW_KEY_LEFT, PEW_KEY_RIGHT

// Modifiers
PEW_KEY_LEFT_SHIFT, PEW_KEY_RIGHT_SHIFT
PEW_KEY_LEFT_CONTROL, PEW_KEY_RIGHT_CONTROL
PEW_KEY_LEFT_ALT, PEW_KEY_RIGHT_ALT
```

### Mouse Button Codes

```cpp
PEW_MOUSE_BUTTON_LEFT    // 0
PEW_MOUSE_BUTTON_RIGHT   // 1
PEW_MOUSE_BUTTON_MIDDLE  // 2
PEW_MOUSE_BUTTON_4       // 3
PEW_MOUSE_BUTTON_5       // 4
// ... up to PEW_MOUSE_BUTTON_8
```

### Events vs Polling

| Approach | Use Case |
|----------|----------|
| **Events** | One-time actions (jump, menu select, pause) |
| **Polling** | Continuous input (movement, camera rotation) |

```cpp
// Events: Respond to key press once
void OnEvent(Event& e) {
    EventDispatcher d(e);
    d.Dispatch<KeyPressedEvent>([](KeyPressedEvent& e) {
        if (e.GetKeyCode() == PEW_KEY_SPACE && e.GetRepeatCount() == 0)
            Jump();  // Only on first press
        return false;
    });
}

// Polling: Check every frame
void OnUpdate(Timestep ts) {
    if (Input::IsKeyPressed(PEW_KEY_W))
        MoveForward(ts);
}
```

---

## Window System

The window system abstracts platform-specific windowing.

### Location
- `PewPew/src/PewPew/Window.h`
- `PewPew/src/Platform/Windows/WindowsWindow.h`

### Window Interface

```cpp
struct WindowProps
{
    String Title;
    unsigned int Width;
    unsigned int Height;

    WindowProps(const String& title = "PewPew Engine",
                unsigned int width = 1280,
                unsigned int height = 720)
        : Title(title), Width(width), Height(height) {}
};

class Window
{
public:
    using EventCallbackFn = std::function<void(Event&)>;

    virtual ~Window() = default;

    virtual void OnUpdate() = 0;
    virtual unsigned int GetWidth() const = 0;
    virtual unsigned int GetHeight() const = 0;

    virtual void SetEventCallback(const EventCallbackFn& callback) = 0;
    virtual void SetVSync(bool enabled) = 0;
    virtual bool IsVSync() const = 0;
    virtual void* GetNativeWindow() const = 0;

    static Scope<Window> Create(const WindowProps& props = WindowProps());
};
```

### Accessing the Window

```cpp
// From application
auto& window = PewPew::Application::Get().GetWindow();

// Get dimensions
unsigned int width = window.GetWidth();
unsigned int height = window.GetHeight();

// Get native handle (GLFWwindow*)
GLFWwindow* glfwWindow = static_cast<GLFWwindow*>(window.GetNativeWindow());

// Toggle VSync
window.SetVSync(true);
```

---

## Logging

The logging system provides formatted output for debugging.

### Location
`PewPew/src/PewPew/Log.h`

### Log Macros

**Engine logging (internal use):**
```cpp
PEW_CORE_TRACE("Detailed info: {0}", value);
PEW_CORE_INFO("General info");
PEW_CORE_WARN("Warning message");
PEW_CORE_ERROR("Error: {0}", errorMsg);
PEW_CORE_CRITICAL("Fatal error!");
```

**Client logging (your application):**
```cpp
PEW_TRACE("Debug: x={0}, y={1}", x, y);
PEW_INFO("Player spawned at ({0}, {1})", pos.x, pos.y);
PEW_WARN("Low health: {0}%", health);
PEW_ERROR("Failed to load: {0}", filename);
PEW_CRITICAL("Out of memory!");
```

### Format Syntax

Uses [fmt](https://fmt.dev) syntax:

```cpp
PEW_INFO("Integer: {0}", 42);
PEW_INFO("Float: {0:.2f}", 3.14159);    // 3.14
PEW_INFO("Hex: {0:#x}", 255);           // 0xff
PEW_INFO("Multiple: {0} + {1} = {2}", a, b, a+b);
```

---

## Profiling

The profiling system captures timing data in Chrome DevTools format.

### Location
`PewPew/src/PewPew/Debug/Instrumentor.h`

### Profile Macros

```cpp
// Profile an entire function
void MyFunction()
{
    PEW_PROFILE_FUNCTION();
    // ... function code
}

// Profile a specific scope
void Update()
{
    {
        PEW_PROFILE_SCOPE("Physics Update");
        UpdatePhysics();
    }

    {
        PEW_PROFILE_SCOPE("AI Update");
        UpdateAI();
    }
}
```

### Session Management

```cpp
// Start profiling session
PEW_PROFILE_BEGIN_SESSION("Startup", "profile-startup.json");
// ... initialization code
PEW_PROFILE_END_SESSION();

// Runtime profiling
PEW_PROFILE_BEGIN_SESSION("Runtime", "profile-runtime.json");
// ... main loop
PEW_PROFILE_END_SESSION();
```

### Viewing Results

1. Open Chrome browser
2. Navigate to `chrome://tracing`
3. Load the generated `.json` file
4. Analyze timing with the visualizer

### ImGui Profiler Panel

Press **F3** to toggle the built-in profiler panel showing:
- Frame time graph
- Per-frame function timings
- FPS counter

---

## Time Management

The `Timestep` class represents frame delta time.

### Location
`PewPew/src/PewPew/Core/TimeStep.h`

### Class Definition

```cpp
class Timestep
{
public:
    Timestep(float time = 0.0f) : m_Time(time) {}

    operator float() const { return m_Time; }

    float GetSeconds() const { return m_Time; }
    float GetMilliseconds() const { return m_Time * 1000.0f; }

private:
    float m_Time;  // In seconds
};
```

### Usage

```cpp
void OnUpdate(PewPew::Timestep ts)
{
    // Implicit conversion to float
    float deltaTime = ts;

    // Frame-rate independent movement
    m_Position += m_Velocity * (float)ts;

    // Or explicitly
    m_Position += m_Velocity * ts.GetSeconds();

    // Debug timing
    PEW_TRACE("Frame time: {0}ms", ts.GetMilliseconds());
}
```

---

## Next Steps

- [Renderer Guide](./RENDERER.md) - Learn 3D rendering
- [API Reference](./API_REFERENCE.md) - Complete API documentation
