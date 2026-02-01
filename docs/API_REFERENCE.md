# API Reference

Quick reference for all public APIs in the PewPew engine.

## Table of Contents

- [Core](#core)
- [Application](#application)
- [Layer](#layer)
- [Events](#events)
- [Input](#input)
- [Window](#window)
- [Renderer](#renderer)
- [Resources](#resources)
- [Camera](#camera)
- [Voxelizer](#voxelizer)
- [Logging](#logging)
- [Profiling](#profiling)
- [Math Types](#math-types)
- [Memory](#memory)

---

## Core

### TimeStep

```cpp
// PewPew/src/PewPew/Core/TimeStep.h
class Timestep
{
    Timestep(float time = 0.0f);
    operator float() const;
    float GetSeconds() const;
    float GetMilliseconds() const;
};
```

### Macros

```cpp
// PewPew/src/PewPew/Core.h
#define BIT(x)                    // (1 << x)
#define PEW_BIND_EVENT_FN(fn)     // Lambda wrapper for member functions
#define PEW_ASSERT(x, ...)        // Client assertion (debug only)
#define PEW_CORE_ASSERT(x, ...)   // Engine assertion (debug only)
```

---

## Application

```cpp
// PewPew/src/PewPew/Application.h
class Application
{
    // Lifecycle
    void Run();
    void Close();

    // Layers
    void PushLayer(Layer* layer);
    void PushOverlay(Layer* overlay);

    // Events
    void OnEvent(Event& e);

    // Accessors
    Window& GetWindow();
    static Application& Get();
};

// Client must implement
PewPew::Application* PewPew::CreateApplication();
```

---

## Layer

```cpp
// PewPew/src/PewPew/Layer.h
class Layer
{
    Layer(const String& name = "Layer");
    virtual ~Layer();

    virtual void OnAttach();
    virtual void OnDetach();
    virtual void OnUpdate(Timestep ts);
    virtual void OnImGuiRender();
    virtual void OnEvent(Event& event);

    const String& GetName() const;
};
```

```cpp
// PewPew/src/PewPew/LayerStack.h
class LayerStack
{
    void PushLayer(Layer* layer);
    void PushOverlay(Layer* overlay);
    void PopLayer(Layer* layer);
    void PopOverlay(Layer* overlay);

    iterator begin();
    iterator end();
};
```

---

## Events

### Base Event

```cpp
// PewPew/src/PewPew/Events/Event.h
class Event
{
    bool Handled;
    virtual EventType GetEventType() const;
    virtual const char* GetName() const;
    virtual int GetCategoryFlags() const;
    bool IsInCategory(EventCategory category);
};
```

### Event Dispatcher

```cpp
class EventDispatcher
{
    EventDispatcher(Event& event);

    template<typename T, typename F>
    bool Dispatch(const F& func);
};
```

### Key Events

```cpp
// PewPew/src/PewPew/Events/KeyEvent.h
class KeyPressedEvent : public KeyEvent
{
    int GetKeyCode() const;
    int GetRepeatCount() const;
};

class KeyReleasedEvent : public KeyEvent
{
    int GetKeyCode() const;
};

class KeyTypedEvent : public KeyEvent
{
    int GetKeyCode() const;
};
```

### Mouse Events

```cpp
// PewPew/src/PewPew/Events/MouseEvent.h
class MouseMovedEvent : public Event
{
    float GetX() const;
    float GetY() const;
};

class MouseScrolledEvent : public Event
{
    float GetXOffset() const;
    float GetYOffset() const;
};

class MouseButtonPressedEvent : public MouseButtonEvent
{
    int GetMouseButton() const;
};

class MouseButtonReleasedEvent : public MouseButtonEvent
{
    int GetMouseButton() const;
};
```

### Application Events

```cpp
// PewPew/src/PewPew/Events/ApplicationEvent.h
class WindowResizeEvent : public Event
{
    unsigned int GetWidth() const;
    unsigned int GetHeight() const;
};

class WindowCloseEvent : public Event {};
```

---

## Input

```cpp
// PewPew/src/PewPew/Input.h
class Input
{
    static bool IsKeyPressed(int keycode);
    static bool IsMouseButtonPressed(int button);
    static std::pair<float, float> GetMousePosition();
    static float GetMouseX();
    static float GetMouseY();
};
```

### Key Codes (Common)

```cpp
// PewPew/src/PewPew/KeyCodes.h
PEW_KEY_SPACE, PEW_KEY_ESCAPE, PEW_KEY_ENTER, PEW_KEY_TAB
PEW_KEY_A - PEW_KEY_Z
PEW_KEY_0 - PEW_KEY_9
PEW_KEY_F1 - PEW_KEY_F12
PEW_KEY_UP, PEW_KEY_DOWN, PEW_KEY_LEFT, PEW_KEY_RIGHT
PEW_KEY_LEFT_SHIFT, PEW_KEY_LEFT_CONTROL, PEW_KEY_LEFT_ALT
```

### Mouse Button Codes

```cpp
// PewPew/src/PewPew/MouseButtonCodes.h
PEW_MOUSE_BUTTON_LEFT   // 0
PEW_MOUSE_BUTTON_RIGHT  // 1
PEW_MOUSE_BUTTON_MIDDLE // 2
```

---

## Window

```cpp
// PewPew/src/PewPew/Window.h
struct WindowProps
{
    String Title = "PewPew Engine";
    unsigned int Width = 1280;
    unsigned int Height = 720;
};

class Window
{
    virtual void OnUpdate();
    virtual unsigned int GetWidth() const;
    virtual unsigned int GetHeight() const;
    virtual void SetEventCallback(const EventCallbackFn& callback);
    virtual void SetVSync(bool enabled);
    virtual bool IsVSync() const;
    virtual void* GetNativeWindow() const;

    static Scope<Window> Create(const WindowProps& props = WindowProps());
};
```

---

## Renderer

### RenderCommand

```cpp
// PewPew/src/PewPew/Renderer/Core/RenderCommand.h
class RenderCommand
{
    static void Init();
    static void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height);
    static void SetClearColor(const Vector4& color);
    static void Clear();
    static void DrawIndexed(const Ref<VertexArray>& vertexArray);
};
```

### Renderer3D

```cpp
// PewPew/src/PewPew/Renderer/Core/Renderer3D.h
class Renderer3D
{
    static void Init();
    static void Shutdown();

    static void BeginScene(const Camera& camera, const Vector3& cameraPosition);
    static void EndScene();

    static void Submit(const Ref<Shader>& shader,
                       const Ref<Material>& material,
                       const Ref<Mesh>& mesh,
                       const Mat4& transform = Mat4(1.0f));

    static void SetDirectionalLight(const Vector3& direction,
                                    const Vector3& color,
                                    float intensity);
    static void SetAmbientLight(const Vector3& color);
};
```

### Renderer (2D/Basic)

```cpp
// PewPew/src/PewPew/Renderer/Core/Renderer.h
class Renderer
{
    static void Init();
    static void Shutdown();
    static void OnWindowResize(uint32_t width, uint32_t height);

    static void BeginScene(const Camera& camera);
    static void EndScene();

    static void Submit(const Ref<Shader>& shader,
                       const Ref<VertexArray>& vertexArray,
                       const Mat4& transform = Mat4(1.0f));

    static RendererAPI::API GetAPI();
};
```

---

## Resources

### Shader

```cpp
// PewPew/src/PewPew/Renderer/Resources/Shader.h
class Shader
{
    virtual void Bind() const;
    virtual void Unbind() const;
    virtual const String& GetName() const;

    void SetInt(const String& name, int value);
    void SetFloat(const String& name, float value);
    void SetFloat2(const String& name, const Vector2& value);
    void SetFloat3(const String& name, const Vector3& value);
    void SetFloat4(const String& name, const Vector4& value);
    void SetMat3(const String& name, const Mat3& matrix);
    void SetMat4(const String& name, const Mat4& matrix);

    static Ref<Shader> Create(const String& filepath);
    static Ref<Shader> Create(const String& name,
                              const String& vertexSrc,
                              const String& fragmentSrc);
};

class ShaderLibrary
{
    void Add(const String& name, const Ref<Shader>& shader);
    void Add(const Ref<Shader>& shader);
    Ref<Shader> Load(const String& filepath);
    Ref<Shader> Load(const String& name, const String& filepath);
    Ref<Shader> Get(const String& name);
    bool Exists(const String& name) const;
};
```

### Texture

```cpp
// PewPew/src/PewPew/Renderer/Resources/Texture.h
class Texture
{
    virtual uint32_t GetWidth() const;
    virtual uint32_t GetHeight() const;
    virtual void Bind(uint32_t slot = 0) const;
};

class Texture2D : public Texture
{
    static Ref<Texture2D> Create(String path);
};
```

### Material

```cpp
// PewPew/src/PewPew/Renderer/Resources/Material.h
class Material
{
    // Texture maps
    void SetAlbedoMap(const Ref<Texture2D>& texture);
    void SetNormalMap(const Ref<Texture2D>& texture);
    void SetMetallicMap(const Ref<Texture2D>& texture);
    void SetRoughnessMap(const Ref<Texture2D>& texture);

    Ref<Texture2D> GetAlbedoMap() const;
    Ref<Texture2D> GetNormalMap() const;
    Ref<Texture2D> GetMetallicMap() const;
    Ref<Texture2D> GetRoughnessMap() const;

    bool HasAlbedoMap() const;
    bool HasNormalMap() const;
    bool HasMetallicMap() const;
    bool HasRoughnessMap() const;

    // Scalar properties
    void SetAlbedo(const Vector3& color);
    void SetMetallic(float value);
    void SetRoughness(float value);
    void SetSmoothShading(float value);

    Vector3 GetAlbedo() const;
    float GetMetallic() const;
    float GetRoughness() const;
    float GetSmoothShading() const;

    // Binding
    void Bind(const Ref<Shader>& shader) const;

    static Ref<Material> Create();
};
```

### Mesh

```cpp
// PewPew/src/PewPew/Renderer/Resources/Mesh.h
struct Vertex
{
    Vector3 Position;
    Vector3 Normal;
    Vector2 TexCoords;
    Vector3 Tangent;
    Vector3 Bitangent;
    Vector3 Color = Vector3(1.0f);
};

class Mesh
{
    Mesh(const std::vector<Vertex>& vertices,
         const std::vector<uint32_t>& indices);

    void Bind() const;
    uint32_t GetIndexCount() const;
    const Ref<VertexArray>& GetVertexArray() const;

    static Ref<Mesh> Load(const String& filePath);
};
```

### Buffers

```cpp
// PewPew/src/PewPew/Renderer/Resources/Buffer.h
enum class ShaderDataType
{
    Float, Float2, Float3, Float4,
    Mat3, Mat4,
    Int, Int2, Int3, Int4,
    Bool
};

struct BufferElement
{
    String Name;
    ShaderDataType Type;
    uint32_t Size;
    uint32_t Offset;
    bool Normalized;

    uint32_t GetComponentCount() const;
};

class BufferLayout
{
    BufferLayout(std::initializer_list<BufferElement> elements);

    uint32_t GetStride() const;
    const std::vector<BufferElement>& GetElements() const;
};

class VertexBuffer
{
    virtual void Bind() const;
    virtual void Unbind() const;
    virtual const BufferLayout& GetLayout() const;
    virtual void SetLayout(const BufferLayout& layout);

    static Ref<VertexBuffer> Create(float* vertices, uint32_t size);
};

class IndexBuffer
{
    virtual void Bind() const;
    virtual void Unbind() const;
    virtual uint32_t GetCount() const;

    static Ref<IndexBuffer> Create(uint32_t* indices, uint32_t count);
};
```

### VertexArray

```cpp
// PewPew/src/PewPew/Renderer/Resources/VertexArray.h
class VertexArray
{
    virtual void Bind() const;
    virtual void Unbind() const;

    virtual void AddVertexBuffer(const Ref<VertexBuffer>& vertexBuffer);
    virtual void SetIndexBuffer(const Ref<IndexBuffer>& indexBuffer);

    virtual const std::vector<Ref<VertexBuffer>>& GetVertexBuffers() const;
    virtual const Ref<IndexBuffer>& GetIndexBuffer() const;

    static Ref<VertexArray> Create();
};
```

---

## Camera

### Camera Base

```cpp
// PewPew/src/PewPew/Renderer/Camera/Camera.h
class Camera
{
    virtual const Mat4& GetProjectionMatrix() const;
    virtual const Mat4& GetViewMatrix() const;
    virtual const Mat4& GetViewProjectionMatrix() const;
};
```

### PerspectiveCamera

```cpp
// PewPew/src/PewPew/Renderer/Camera/PerspectiveCamera.h
class PerspectiveCamera : public Camera
{
    PerspectiveCamera(float fovDegrees, float aspectRatio,
                      float nearClip, float farClip);

    void SetProjection(float fovDegrees, float aspectRatio,
                       float nearClip, float farClip);

    const Vector3& GetPosition() const;
    void SetPosition(const Vector3& position);

    float GetPitch() const;
    float GetYaw() const;
    void SetRotation(float pitch, float yaw);

    Vector3 GetForwardDirection() const;
    Vector3 GetRightDirection() const;
    Vector3 GetUpDirection() const;

    const Mat4& GetProjectionMatrix() const override;
    const Mat4& GetViewMatrix() const override;
    const Mat4& GetViewProjectionMatrix() const override;
};
```

### CameraController

```cpp
// PewPew/src/PewPew/Renderer/Camera/CameraController.h
class CameraController
{
    virtual void OnUpdate(Timestep ts);
    virtual void OnEvent(Event& e);
    virtual Camera& GetCamera();
    virtual const Camera& GetCamera() const;
    virtual void SetViewportSize(float width, float height);
    virtual void SetEnabled(bool enabled);
    bool IsEnabled() const;
};
```

### PerspectiveCameraController

```cpp
// PewPew/src/PewPew/Renderer/Camera/PerspectiveCameraController.h
class PerspectiveCameraController : public CameraController
{
    PerspectiveCameraController(float fovDegrees, float aspectRatio,
                                float nearClip, float farClip,
                                bool enableRotation = true);

    void OnUpdate(Timestep ts) override;
    void OnEvent(Event& e) override;
    void SetViewportSize(float width, float height) override;

    PerspectiveCamera& GetCamera();
    const PerspectiveCamera& GetCamera() const;
};
```

---

## Voxelizer

```cpp
// PewPew/src/PewPew/Renderer/VoxelizerAPI.h
struct VoxelizeSettings
{
    int GridSize = 32;      // 8-128
    float VoxelSize = 0.0f; // If > 0, overrides GridSize
    bool Solid = true;      // Fill interior
    float Padding = 0.01f;  // World units
};

struct VoxelMeshData
{
    Ref<Mesh> Mesh;
    voxelizer::VoxelGrid Grid;
    uint64_t VoxelCount;
};

class VoxelizerAPI
{
    static void Init();
    static void Shutdown();

    // Voxelization
    static VoxelMeshData Voxelize(const Ref<Mesh>& mesh,
                                  const std::vector<Vertex>& vertices,
                                  const std::vector<uint32_t>& indices,
                                  const VoxelizeSettings& settings = {});

    static VoxelMeshData Voxelize(const std::vector<Vertex>& vertices,
                                  const std::vector<uint32_t>& indices,
                                  const VoxelizeSettings& settings = {});

    static VoxelMeshData VoxelizeFromFile(const String& filePath,
                                          const VoxelizeSettings& settings = {});

    // With texture colors
    static VoxelMeshData VoxelizeWithColors(const std::vector<Vertex>& vertices,
                                            const std::vector<uint32_t>& indices,
                                            const String& texturePath,
                                            const VoxelizeSettings& settings = {});

    static VoxelMeshData VoxelizeFromFileWithColors(const String& meshPath,
                                                    const String& texturePath,
                                                    const VoxelizeSettings& settings = {});

    // Mesh creation
    static Ref<Mesh> CreateMeshFromGrid(const voxelizer::VoxelGrid& grid);
    static Ref<Mesh> CreateMeshFromGrid(const voxelizer::VoxelGrid& grid,
                                        const Vector3& color);
    static Ref<Mesh> CreateColoredMeshFromGrid(const voxelizer::VoxelGrid& grid,
                                               const std::vector<Vector3>& colors);

    // Queries
    static std::vector<Vector3> GetVoxelPositions(const voxelizer::VoxelGrid& grid);
    static Vector3 GetVoxelSize(const voxelizer::VoxelGrid& grid);
    static bool IsPointInside(const voxelizer::VoxelGrid& grid,
                              const Vector3& worldPos);

    // Helpers
    static Ref<Shader> GetVoxelShader();
    static Ref<Material> CreateVoxelMaterial(const Vector3& color = {0.8f, 0.8f, 0.8f});
};
```

---

## Logging

```cpp
// PewPew/src/PewPew/Log.h
class Log
{
    static void Init();
    static Ref<spdlog::logger>& GetCoreLogger();
    static Ref<spdlog::logger>& GetClientLogger();
};

// Engine macros
PEW_CORE_TRACE(...)
PEW_CORE_INFO(...)
PEW_CORE_WARN(...)
PEW_CORE_ERROR(...)
PEW_CORE_CRITICAL(...)

// Client macros
PEW_TRACE(...)
PEW_INFO(...)
PEW_WARN(...)
PEW_ERROR(...)
PEW_CRITICAL(...)
```

---

## Profiling

```cpp
// PewPew/src/PewPew/Debug/Instrumentor.h

// Session management
PEW_PROFILE_BEGIN_SESSION(name, filepath)
PEW_PROFILE_END_SESSION()

// Scope profiling
PEW_PROFILE_FUNCTION()    // Profile current function
PEW_PROFILE_SCOPE(name)   // Profile named scope

// Frame tracking
PEW_PROFILE_BEGIN_FRAME()
PEW_PROFILE_END_FRAME()
```

---

## Math Types

```cpp
// PewPew/src/PewPew/Math/Math.h (aliases for glm)
using Vector2 = glm::vec2;
using Vector3 = glm::vec3;
using Vector4 = glm::vec4;
using Mat3 = glm::mat3;
using Mat4 = glm::mat4;
using Quat = glm::quat;
```

### Common Operations

```cpp
// Vector operations
glm::vec3 a = {1.0f, 2.0f, 3.0f};
glm::vec3 b = glm::normalize(a);
float len = glm::length(a);
float d = glm::dot(a, b);
glm::vec3 c = glm::cross(a, b);

// Matrix operations
glm::mat4 identity = glm::mat4(1.0f);
glm::mat4 translated = glm::translate(identity, {x, y, z});
glm::mat4 rotated = glm::rotate(identity, glm::radians(45.0f), {0, 1, 0});
glm::mat4 scaled = glm::scale(identity, {sx, sy, sz});

// Perspective projection
glm::mat4 proj = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 1000.0f);

// View matrix
glm::mat4 view = glm::lookAt(eye, target, up);
```

---

## Memory

```cpp
// PewPew/src/PewPew/Core.h

// Unique pointer (single ownership)
template<typename T>
using Scope = std::unique_ptr<T>;

template<typename T, typename... Args>
constexpr Scope<T> CreateScope(Args&&... args);

// Shared pointer (reference counted)
template<typename T>
using Ref = std::shared_ptr<T>;

template<typename T, typename... Args>
constexpr Ref<T> CreateRef(Args&&... args);
```

### Usage

```cpp
// Unique ownership
Scope<Window> window = CreateScope<WindowsWindow>(props);

// Shared ownership
Ref<Shader> shader = CreateRef<OpenGLShader>(filepath);
Ref<Mesh> mesh = Mesh::Load("model.fbx");  // Factory returns Ref
```

---

## Platform Defines

```cpp
// Build configuration
PEW_DEBUG       // Debug build
PEW_RELEASE     // Release build
PEW_DIST        // Distribution build

// Platform
PEW_PLATFORM_WINDOWS

// Internal
PEW_BUILD_DLL
PEW_ENABLE_ASSERTS  // Enables assertions (debug only)
```
