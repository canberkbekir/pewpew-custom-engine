# Entity Picking via GPU ID Buffer

Select entities by clicking them in the viewport using a second framebuffer color attachment that stores entity IDs per-pixel.

## Approach: Single-pass MRT (Multiple Render Targets)

Render entity IDs alongside the color output in the same draw call. On left-click, read back the entity ID at the clicked pixel and call `Selection::Select()`.

**Why single-pass?** No extra draw calls, no separate shader. Just add a second output to the existing PBR shader and a second color attachment to the framebuffer.

---

## Files to Modify

### CBEngine (core engine library) — 4 files

#### 1. `CBEngine/src/CBEngine/Renderer/Core/Framebuffer.h`

- Add `FramebufferTextureFormat` enum: `RGBA8`, `RED_INTEGER`, `DEPTH24_STENCIL8`
- Add `FramebufferTextureSpecification` and `FramebufferAttachmentSpecification` structs
- Add `Attachments` field to `FramebufferSpecification`
- Change `GetColorAttachmentRendererID()` to take `uint32_t index = 0`
- Add `ReadPixel(uint32_t attachmentIndex, int x, int y) -> int`
- Add `ClearAttachment(uint32_t attachmentIndex, int value)`

#### 2. `CBEngine/src/Platform/OpenGL/OpenGLFramebuffer.h` + `.cpp`

- Replace single `m_ColorAttachment` with `std::vector<uint32_t> m_ColorAttachments`
- Add `std::vector<FramebufferTextureSpecification> m_ColorAttachmentSpecs`
- `Invalidate()`: create textures per-spec, `GL_RGBA8` for RGBA8, `GL_R32I` for RED_INTEGER, call `glDrawBuffers()` for MRT
- `ReadPixel()`: `glReadBuffer(GL_COLOR_ATTACHMENT0 + index)` then `glReadPixels(..., GL_RED_INTEGER, GL_INT, ...)`
- `ClearAttachment()`: `glClearBufferiv(GL_COLOR, index, &value)` for integer attachments
- Constructor: parse `Attachments` spec, separating color vs depth

#### 3. `CBEngine/src/CBEngine/Renderer/Core/ShaderUniforms.h`

- Add: `inline constexpr auto EntityID = "u_EntityID";`

#### 4. `CBEngine/src/CBEngine/Renderer/Core/Renderer3D.h` + `.cpp`

- Add overload: `Submit(..., const Mat4& transform, int entityID)`
- Uploads `u_EntityID` uniform via `shader->SetInt(EntityID, entityID)`
- Refactor original `Submit()` to call the new overload with `-1`

---

### CBEngine-Editor — 2 files

#### 5. `CBEngine-Editor/assets/shaders/PBR.glsl` (fragment shader)

- Change `out vec4 FragColor;` → `layout(location = 0) out vec4 FragColor;`
- Add `layout(location = 1) out int o_EntityID;`
- Add `uniform int u_EntityID;`
- At end of `main()`: `o_EntityID = u_EntityID;`

#### 6. `CBEngine-Editor/src/Panels/ViewportPanel.h` + `.cpp`

- Add `Vector2 m_ViewportBoundsMin`, `m_ViewportBoundsMax` members
- Constructor: update framebuffer spec to `{ RGBA8, RED_INTEGER, Depth }`
- `RenderScene()`: call `m_Framebuffer->ClearAttachment(1, -1)` before rendering; pass `static_cast<int>(entityID)` to `Renderer3D::Submit()`
- `OnImGuiRender()`: after `ImGui::Image()`, capture viewport bounds via `ImGui::GetItemRectMin/Max()`. On left-click: convert mouse pos to framebuffer coords (flip Y for OpenGL), call `ReadPixel(1, x, y)`, then `Selection::Select(Selectable::Entity(uuid))` or `Selection::Clear()` for `-1`
- Add includes for `Selection.h` and `Selectable.h`

---

## Implementation Order

1. **Framebuffer.h** — new types and interface
2. **OpenGLFramebuffer.h/.cpp** — implement multi-attachment + ReadPixel + ClearAttachment
3. **ShaderUniforms.h** — add EntityID constant
4. **Renderer3D.h/.cpp** — add Submit overload with entity ID
5. **PBR.glsl** — add second output
6. **ViewportPanel.h/.cpp** — wire up framebuffer spec, entity ID passing, and click-to-pick

---

## Key Details

| Detail | Description |
|--------|-------------|
| **Sentinel value** | `-1` = no entity (maps naturally to `entt::null`) |
| **Y-flip** | ImGui origin is top-left, framebuffer origin is bottom-left → `mouseY = viewportHeight - mouseY` |
| **Toolbar protection** | Use `ImGui::GetItemRectMin/Max()` right after `ImGui::Image()` to constrain picking to the viewport image only |
| **Camera conflict** | Picking uses left-click, camera uses right-click — no conflict |
| **ReadPixel requires bind** | Bind framebuffer before `ReadPixel`, unbind after |

---

## Verification

1. Run `GenerateProject.bat` to regenerate VS solution
2. Build Debug configuration
3. Load a scene with multiple entities
4. Left-click on an entity mesh → Properties panel should show that entity
5. Left-click on empty space → selection should clear
6. Verify camera right-click rotation still works independently
7. Verify clicking ImGui widgets (toolbar sliders etc.) doesn't trigger picking