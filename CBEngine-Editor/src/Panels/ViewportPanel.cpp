#include "ViewportPanel.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <ImGuizmo/ImGuizmo.h>

#include <glm/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>

#include "CBEngine/Asset/AssetManager.h"
#include "CBEngine/Asset/ProcessedMeshAsset.h"
#include "CBEngine/Asset/BlueprintAsset.h"
#include "CBEngine/Components/CoreComponents.h"
#include "CBEngine/Components/ColliderComponent.h"
#include "CBEngine/Components/Components.h"
#include "CBEngine/Components/MeshRendererComponent.h"
#include "CBEngine/Components/VoxelRendererComponent.h"
#include "CBEngine/Components/TransformComponent.h"
#include "CBEngine/Debug/ColliderDebugRenderer.h"
#include "CBEngine/Debug/Instrumentor.h"
#include "CBEngine/Renderer/Core/RenderCommand.h"
#include "CBEngine/Scene/SceneManager.h"
#include "CBEngine/Scene/SceneSerializer.h"
#include "CBEngine/Scene/Scene.h"
#include "CBEngine/Scene/Entity.h"
#include "CBEngine/Selection/Selection.h"
#include "CBEngine/Systems/RendererSystem.h"

namespace CB
{
    //--------------------------------------------------------------------------
    // Construction
    //--------------------------------------------------------------------------

    ViewportPanel::ViewportPanel()
        : Panel("Viewport", true)
          , m_CameraController(45.0f, 1280.0f / 720.0f, 0.1f, 100.0f)
    {
        // Create framebuffer with entity ID attachment for picking
        FramebufferSpecification fbSpec;
        fbSpec.Attachments = {
            {FramebufferTextureFormat::RGBA8}, {FramebufferTextureFormat::RED_INTEGER},
            {FramebufferTextureFormat::Depth}
        };
        fbSpec.Width = 1280;
        fbSpec.Height = 720;
        m_Framebuffer = Framebuffer::Create(fbSpec);

        // Load default shader
        m_DefaultShader = Shader::Create("assets/shaders/PBR.glsl");

        // Create default material
        m_DefaultMaterial = CreateRef<Material>();
        m_DefaultMaterial->SetAlbedo({0.8f, 0.8f, 0.8f});
        m_DefaultMaterial->SetRoughness(0.5f);
        m_DefaultMaterial->SetMetallic(0.0f);
    }

    //--------------------------------------------------------------------------
    // Main Update Loop
    //--------------------------------------------------------------------------

    void ViewportPanel::OnUpdate(Timestep ts)
    {
        CB_PROFILE_FUNCTION();

        // Resize framebuffer if needed
        auto spec = m_Framebuffer->GetSpecification();
        if (m_ViewportSize.x > 0.0f && m_ViewportSize.y > 0.0f)
        {
            if (spec.Width != static_cast<uint32_t>(m_ViewportSize.x) ||
                spec.Height != static_cast<uint32_t>(m_ViewportSize.y))
            {
                m_Framebuffer->Resize(
                    static_cast<uint32_t>(m_ViewportSize.x),
                    static_cast<uint32_t>(m_ViewportSize.y)
                );
                m_CameraController.SetViewportSize(m_ViewportSize.x, m_ViewportSize.y);
            }
        }

        // Update camera only when focused and not using gizmo
        if (m_Focused && !ImGuizmo::IsUsing())
            m_CameraController.OnUpdate(ts);

        // Render to framebuffer
        m_Framebuffer->Bind();
        RenderScene();
        m_Framebuffer->Unbind();
    }

    void ViewportPanel::RenderScene()
    {
        // Clear
        RenderCommand::SetClearColor({0.15f, 0.15f, 0.18f, 1.0f});
        RenderCommand::Clear();

        // Clear entity ID attachment to -1 (no entity)
        m_Framebuffer->ClearAttachment(1, -1);

        // Wireframe mode
        RenderCommand::SetWireframeMode(m_Wireframe);

        // Render via RendererSystem
        Ref<Scene> scene = SceneManager::GetActiveScene();
        RendererSystem::OnUpdate(
            scene.get(),
            m_CameraController.GetCamera(),
            m_CameraController.GetCamera().GetPosition(),
            m_DefaultShader,
            m_DefaultMaterial
        );

        // Restore fill mode
        RenderCommand::SetWireframeMode(false);

        // Draw collider wireframes
        if (scene)
        {
            if (m_ShowColliders)
            {
                // Show ALL colliders in the scene (no recursion since view covers all)
                auto view = scene->GetRegistry().view<ColliderComponent, TransformComponent>();
                for (auto entityHandle : view)
                {
                    Entity entity = { entityHandle, scene.get() };
                    DrawEntityColliders(entity, false);
                }
            }
            else if (Selection::HasEntitySelected())
            {
                // Show collider only for the selected entity (even when toggle is off)
                UUID entityUUID = Selection::GetPrimarySelection().ID;
                Entity entity = scene->GetEntityByUUID(entityUUID);
                if (entity)
                    DrawEntityColliders(entity);
            }
        }
    }

    //--------------------------------------------------------------------------
    // ImGui Rendering
    //--------------------------------------------------------------------------

    void ViewportPanel::OnImGuiRender()
    {
        if (!m_Visible)
            return;

        // No padding for viewport image
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::Begin(m_Name.c_str(), &m_Visible);

        m_Focused = ImGui::IsWindowFocused();
        m_Hovered = ImGui::IsWindowHovered();

        // Toolbar
        RenderToolbar();

        // Viewport size
        ImVec2 availableSize = ImGui::GetContentRegionAvail();
        m_ViewportSize = {availableSize.x, availableSize.y};

        // Display framebuffer texture (UV flipped for OpenGL)
        uint64_t textureID = m_Framebuffer->GetColorAttachmentRendererID();
        ImGui::Image(
            reinterpret_cast<void*>(textureID),
            availableSize,
            ImVec2(0, 1),
            ImVec2(1, 0)
        );

        // Drag-drop target on viewport
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
            {
                std::filesystem::path droppedPath(static_cast<const char*>(payload->Data));
                std::string ext = droppedPath.extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

                Ref<Scene> scene = SceneManager::GetActiveScene();
                if (scene)
                {
                    if (ext == ".blueprint")
                    {
                        auto bp = BlueprintAsset::Load(droppedPath);
                        if (bp)
                        {
                            SceneSerializer serializer(scene);
                            std::filesystem::path relPath = relative(droppedPath, std::filesystem::path("assets"));
                            UUID bpUUID = AssetManager::GetRegistry().GetUUIDByPath(relPath);
                            serializer.InstantiateBlueprint(bp->YAMLData, droppedPath.string(), bpUUID);
                        }
                    }
                    else if (ext == ".mesh")
                    {
                        std::filesystem::path relPath = relative(droppedPath, std::filesystem::path("assets"));
                        UUID meshUUID = AssetManager::GetRegistry().GetUUIDByPath(relPath);
                        if (meshUUID.IsValid())
                        {
                            String name = droppedPath.stem().string();
                            Entity entity = scene->CreateEntity(name);
                            auto& mrc = entity.AddComponent<MeshRendererComponent>();
                            mrc.MeshUUID = meshUUID;
                            mrc.ResolveAssets();
                            Selection::Select(Selectable::Entity(entity.GetUUID()));
                        }
                    }
                    else if (ext == ".vmesh")
                    {
                        std::filesystem::path relPath = relative(droppedPath, std::filesystem::path("assets"));
                        UUID vmeshUUID = AssetManager::GetRegistry().GetUUIDByPath(relPath);
                        if (vmeshUUID.IsValid())
                        {
                            String name = droppedPath.stem().string();
                            Entity entity = scene->CreateEntity(name);
                            auto& vrc = entity.AddComponent<VoxelRendererComponent>();
                            vrc.VoxelMeshUUID = vmeshUUID;
                            vrc.ResolveAssets();
                            Selection::Select(Selectable::Entity(entity.GetUUID()));
                        }
                    }
                    else if (ext == ".mat")
                    {
                        // Apply material to selected entity
                        if (Selection::HasEntitySelected())
                        {
                            UUID entityUUID = Selection::GetPrimarySelection().ID;
                            Entity entity = scene->GetEntityByUUID(entityUUID);
                            if (entity && entity.HasComponent<MeshRendererComponent>())
                            {
                                std::filesystem::path relPath = relative(droppedPath, std::filesystem::path("assets"));
                                UUID matUUID = AssetManager::GetRegistry().GetUUIDByPath(relPath);
                                if (matUUID.IsValid())
                                {
                                    auto& mrc = entity.GetComponent<MeshRendererComponent>();
                                    mrc.MaterialUUID = matUUID;
                                    mrc.ResolveAssets();
                                }
                            }
                        }
                    }
                    else if (ext == ".scene")
                    {
                        SceneManager::LoadScene(droppedPath.string());
                    }
                    else if (ext == ".fbx" || ext == ".obj" || ext == ".gltf" || ext == ".glb")
                    {
                        // Raw meshes need import; create entity with MeshRendererComponent placeholder
                        String name = droppedPath.stem().string();
                        Entity entity = scene->CreateEntity(name);
                        entity.AddComponent<MeshRendererComponent>();
                        Selection::Select(Selectable::Entity(entity.GetUUID()));
                        CB_CORE_WARN("Dropped raw mesh '{0}' - use Import Panel to process into .mesh first", name);
                    }
                    else if (ext == ".lua")
                    {
                        // Assign script to selected entity
                        if (Selection::HasEntitySelected())
                        {
                            UUID entityUUID = Selection::GetPrimarySelection().ID;
                            Entity entity = scene->GetEntityByUUID(entityUUID);
                            if (entity)
                            {
                                if (!entity.HasComponent<ScriptComponent>())
                                    entity.AddComponent<ScriptComponent>();
                                entity.GetComponent<ScriptComponent>().ScriptPath = droppedPath.string();
                            }
                        }
                    }
                }
            }
            ImGui::EndDragDropTarget();
        }

        // Entity picking (before gizmo so we can check ImGuizmo::IsOver)
        HandleEntityPicking();

        // Gizmo overlay
        RenderGizmo();

        // Keyboard shortcuts for gizmo modes (when viewport focused and not typing)
        if (m_Focused && !ImGui::GetIO().WantTextInput)
        {
            if (ImGui::IsKeyPressed(ImGuiKey_W))
                m_GizmoOperation = ImGuizmo::TRANSLATE;
            if (ImGui::IsKeyPressed(ImGuiKey_E))
                m_GizmoOperation = ImGuizmo::ROTATE;
            if (ImGui::IsKeyPressed(ImGuiKey_R))
                m_GizmoOperation = ImGuizmo::SCALE;
        }

        // Stats overlay
        if (m_ShowStats)
            RenderStatsOverlay();

        ImGui::End();
        ImGui::PopStyleVar();
    }

    void ViewportPanel::RenderToolbar()
    {
        // Toolbar styling - 1.5x height
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 6));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 6));
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.12f, 0.12f, 0.14f, 1.0f));

        float toolbarHeight = ImGui::GetFrameHeight() + 12.0f;
        ImGui::BeginChild("##Toolbar", ImVec2(0, toolbarHeight), false, ImGuiWindowFlags_NoScrollbar);

        // Center content vertically
        float contentHeight = ImGui::GetFrameHeight();
        ImGui::SetCursorPosY((toolbarHeight - contentHeight) * 0.5f);
        ImGui::SetCursorPosX(8.0f);

        // Gizmo mode buttons
        if (ImGui::Selectable("T", m_GizmoOperation == ImGuizmo::TRANSLATE, 0, ImVec2(20, 0)))
            m_GizmoOperation = ImGuizmo::TRANSLATE;
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Translate (W)");

        ImGui::SameLine();
        if (ImGui::Selectable("R", m_GizmoOperation == ImGuizmo::ROTATE, 0, ImVec2(20, 0)))
            m_GizmoOperation = ImGuizmo::ROTATE;
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Rotate (E)");

        ImGui::SameLine();
        if (ImGui::Selectable("S", m_GizmoOperation == ImGuizmo::SCALE, 0, ImVec2(20, 0)))
            m_GizmoOperation = ImGuizmo::SCALE;
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Scale (R)");

        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();

        // Transform space toggle
        bool isLocal = (m_GizmoMode == ImGuizmo::LOCAL);
        if (ImGui::Selectable("Local", isLocal, 0, ImVec2(40, 0)))
            m_GizmoMode = ImGuizmo::LOCAL;
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Local space");

        ImGui::SameLine();
        if (ImGui::Selectable("World", !isLocal, 0, ImVec2(40, 0)))
            m_GizmoMode = ImGuizmo::WORLD;
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("World space");

        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();

        // Camera Speed
        ImGui::Text("Speed");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(80);
        float speed = m_CameraController.GetMoveSpeed();
        if (ImGui::DragFloat("##Speed", &speed, 0.1f, 0.1f, 50.0f, "%.1f"))
            m_CameraController.SetMoveSpeed(speed);

        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();

        // FOV
        ImGui::Text("FOV");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(60);
        float fov = m_CameraController.GetFOV();
        if (ImGui::DragFloat("##FOV", &fov, 0.5f, 10.0f, 120.0f, "%.0f"))
            m_CameraController.SetFOV(fov);

        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();

        // Near/Far Clip
        ImGui::Text("Near");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(60);
        float nearClip = m_CameraController.GetNearClip();
        if (ImGui::DragFloat("##Near", &nearClip, 0.01f, 0.001f, 10.0f, "%.2f"))
            m_CameraController.SetNearClip(nearClip);

        ImGui::SameLine();
        ImGui::Text("Far");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(70);
        float farClip = m_CameraController.GetFarClip();
        if (ImGui::DragFloat("##Far", &farClip, 1.0f, 10.0f, 10000.0f, "%.0f"))
            m_CameraController.SetFarClip(farClip);

        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();

        // Camera Position (read-only)
        auto pos = m_CameraController.GetCamera().GetPosition();
        ImGui::TextDisabled("Pos: %.1f, %.1f, %.1f", pos.x, pos.y, pos.z);

        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();

        // Toggles
        if (ImGui::Selectable("Wireframe", m_Wireframe, 0, ImVec2(70, 0)))
            m_Wireframe = !m_Wireframe;

        ImGui::SameLine();
        if (ImGui::Selectable("Colliders", m_ShowColliders, 0, ImVec2(60, 0)))
            m_ShowColliders = !m_ShowColliders;
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Show all colliders in scene (off = selected entity only)");

        ImGui::SameLine();
        if (ImGui::Selectable("Stats", m_ShowStats, 0, ImVec2(40, 0)))
            m_ShowStats = !m_ShowStats;

        // --- Simulation Controls (centered-ish, right side) ---
        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();

        {
            Ref<Scene> scene = SceneManager::GetActiveScene();
            bool simulating = scene && scene->IsPhysicsInitialized();
            bool paused = scene && scene->IsPhysicsPaused();

            if (!simulating)
            {
                // Play button
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 0.8f, 0.2f, 1.0f));
                if (ImGui::Selectable("|>", false, 0, ImVec2(20, 0)))
                {
                    if (scene)
                        scene->InitPhysics();
                }
                ImGui::PopStyleColor();
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Play (F5)");
            }
            else
            {
                // Stop button
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.2f, 0.2f, 1.0f));
                if (ImGui::Selectable("[]", false, 0, ImVec2(20, 0)))
                {
                    scene->ShutdownPhysics();
                }
                ImGui::PopStyleColor();
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Stop (F5)");

                ImGui::SameLine();

                // Pause / Resume button
                if (paused)
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 0.8f, 0.2f, 1.0f));
                    if (ImGui::Selectable("|>##resume", false, 0, ImVec2(20, 0)))
                        scene->ResumePhysics();
                    ImGui::PopStyleColor();
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Resume");
                }
                else
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.0f, 1.0f));
                    if (ImGui::Selectable("||", false, 0, ImVec2(20, 0)))
                        scene->PausePhysics();
                    ImGui::PopStyleColor();
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Pause");
                }

                ImGui::SameLine();

                // Step one frame button
                bool canStep = paused;
                if (!canStep)
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
                if (ImGui::Selectable("|>|", canStep ? false : false, canStep ? 0 : ImGuiSelectableFlags_Disabled, ImVec2(24, 0)))
                {
                    if (canStep)
                        scene->StepOneFrame();
                }
                if (!canStep)
                    ImGui::PopStyleColor();
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Step One Frame (only when paused)");
            }
        }

        ImGui::EndChild();
        ImGui::Separator();

        ImGui::PopStyleColor();
        ImGui::PopStyleVar(2);
    }

    void ViewportPanel::RenderStatsOverlay()
    {
        ImVec2 windowPos = ImGui::GetWindowPos();
        ImVec2 contentMin = ImGui::GetWindowContentRegionMin();

        ImGui::SetNextWindowPos(ImVec2(
            windowPos.x + contentMin.x + 10.0f,
            windowPos.y + contentMin.y + 50.0f
        ));
        ImGui::SetNextWindowBgAlpha(0.7f);

        ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoDocking |
            ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoFocusOnAppearing |
            ImGuiWindowFlags_NoNav |
            ImGuiWindowFlags_NoMove;

        if (ImGui::Begin("##ViewportStats", nullptr, flags))
        {
            ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
            ImGui::Text("Frame: %.2f ms", 1000.0f / ImGui::GetIO().Framerate);

            Ref<Scene> scene = SceneManager::GetActiveScene();
            if (scene)
            {
                auto meshView = scene->GetRegistry().view<MeshRendererComponent>();
                auto voxelView = scene->GetRegistry().view<VoxelRendererComponent>();
                ImGui::Text("Mesh: %d | Voxel: %d",
                            static_cast<int>(meshView.size()),
                            static_cast<int>(voxelView.size()));
            }

            ImGui::Text("Viewport: %dx%d",
                        static_cast<int>(m_ViewportSize.x),
                        static_cast<int>(m_ViewportSize.y)
            );

            auto pos = m_CameraController.GetCamera().GetPosition();
            ImGui::Text("Camera: %.1f, %.1f, %.1f", pos.x, pos.y, pos.z);
        }
        ImGui::End();
    }

    //--------------------------------------------------------------------------
    // Collider Debug Drawing
    //--------------------------------------------------------------------------

    void ViewportPanel::DrawEntityColliders(Entity entity, bool recurseChildren)
    {
        if (!entity)
            return;

        // Draw this entity's collider if it has one
        if (entity.HasComponent<ColliderComponent>() && entity.HasComponent<TransformComponent>())
        {
            auto& collider = entity.GetComponent<ColliderComponent>();
            auto& tc = entity.GetComponent<TransformComponent>();
            Mat4 worldTransform = tc.HasParent() ? tc.WorldMatrix : tc.GetLocalTransform();

            if (collider.Shape == ColliderShape::VoxelCompound)
            {
                // For voxel compound, draw each merged box from the voxel grid
                if (entity.HasComponent<VoxelRendererComponent>())
                {
                    auto& vrc = entity.GetComponent<VoxelRendererComponent>();
                    if (vrc.VoxelMeshUUID.IsValid())
                    {
                        auto vmesh = AssetManager::GetAsset<VoxelMeshAsset>(vrc.VoxelMeshUUID);
                        if (vmesh && vmesh->GridData.CountFilled() > 0)
                        {
                            ColliderDebugRenderer::DrawVoxelCompound(
                                vmesh->GridData, worldTransform, m_CameraController.GetCamera(),
                                collider.Offset);
                        }
                        else
                        {
                            // Fallback: draw bounding box
                            ColliderDebugRenderer::DrawCollider(
                                collider, worldTransform, m_CameraController.GetCamera(),
                                Vector3(1.0f, 0.5f, 0.0f));
                        }
                    }
                }
            }
            else
            {
                ColliderDebugRenderer::DrawCollider(
                    collider, worldTransform, m_CameraController.GetCamera());
            }
        }

        // Recursively draw children's colliders
        if (recurseChildren && entity.HasChildren())
        {
            for (Entity child : entity.GetChildren())
                DrawEntityColliders(child, true);
        }
    }

    //--------------------------------------------------------------------------
    // Entity Picking
    //--------------------------------------------------------------------------

    void ViewportPanel::HandleEntityPicking()
    {
        // Only pick on left-click, when viewport is hovered, and not using gizmo
        if (!m_Hovered)
            return;
        if (!ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            return;
        if (ImGuizmo::IsOver())
            return;

        // Compute mouse position relative to the framebuffer image
        ImVec2 mousePos = ImGui::GetMousePos();
        ImVec2 imageMin = ImGui::GetItemRectMin();
        ImVec2 imageSize = ImGui::GetItemRectSize();

        float mx = mousePos.x - imageMin.x;
        float my = mousePos.y - imageMin.y;

        // Flip Y for OpenGL (bottom-left origin)
        my = imageSize.y - my;

        int mouseX = static_cast<int>(mx);
        int mouseY = static_cast<int>(my);

        auto spec = m_Framebuffer->GetSpecification();
        if (mouseX >= 0 && mouseY >= 0 && mouseX < static_cast<int>(spec.Width) && mouseY < static_cast<int>(spec.
            Height))
        {
            m_Framebuffer->Bind();
            int pixelData = m_Framebuffer->ReadPixel(1, mouseX, mouseY);
            m_Framebuffer->Unbind();

            if (pixelData >= 0)
            {
                Ref<Scene> scene = SceneManager::GetActiveScene();
                if (scene)
                {
                    auto& registry = scene->GetRegistry();
                    auto entityHandle = static_cast<entt::entity>((uint32_t)pixelData);

                    if (registry.valid(entityHandle) && registry.all_of<IDComponent>(entityHandle))
                    {
                        UUID uuid = registry.get<IDComponent>(entityHandle).ID;
                        Selection::Select(Selectable::Entity(uuid));
                    }
                }
            }
            else
            {
                Selection::Clear();
            }
        }
    }

    //--------------------------------------------------------------------------
    // Gizmo
    //--------------------------------------------------------------------------

    void ViewportPanel::RenderGizmo()
    {
        if (!Selection::HasEntitySelected())
            return;

        Ref<Scene> scene = SceneManager::GetActiveScene();
        if (!scene)
            return;

        UUID entityUUID = Selection::GetPrimarySelection().ID;
        Entity entity = scene->GetEntityByUUID(entityUUID);
        if (!entity || !entity.HasComponent<TransformComponent>())
            return;

        auto& tc = entity.GetComponent<TransformComponent>();

        // Camera matrices
        const Mat4& viewMatrix = m_CameraController.GetCamera().GetViewMatrix();
        const Mat4& projMatrix = m_CameraController.GetCamera().GetProjectionMatrix();

        // Setup ImGuizmo for this window
        ImGuizmo::SetOrthographic(false);
        ImGuizmo::SetDrawlist();

        ImVec2 imageMin = ImGui::GetItemRectMin();
        ImVec2 imageSize = ImGui::GetItemRectSize();
        ImGuizmo::SetRect(imageMin.x, imageMin.y, imageSize.x, imageSize.y);

        // Build entity transform matrix (world space for parented entities)
        Mat4 transformMatrix = tc.HasParent() ? tc.WorldMatrix : tc.GetLocalTransform();

        // Snap values
        float snapValues[3] = {0.0f, 0.0f, 0.0f};
        bool useSnap = ImGui::GetIO().KeyCtrl;
        if (useSnap)
        {
            if (m_GizmoOperation == ImGuizmo::TRANSLATE)
                snapValues[0] = snapValues[1] = snapValues[2] = 0.5f;
            else if (m_GizmoOperation == ImGuizmo::ROTATE)
                snapValues[0] = snapValues[1] = snapValues[2] = 15.0f;
            else if (m_GizmoOperation == ImGuizmo::SCALE)
                snapValues[0] = snapValues[1] = snapValues[2] = 0.1f;
        }

        // Manipulate
        Manipulate(
            value_ptr(viewMatrix),
            value_ptr(projMatrix),
            m_GizmoOperation,
            m_GizmoOperation == ImGuizmo::SCALE ? ImGuizmo::LOCAL : m_GizmoMode,
            value_ptr(transformMatrix),
            nullptr,
            useSnap ? snapValues : nullptr
        );

        if (ImGuizmo::IsUsing())
        {
            // If entity has a parent, convert world-space matrix back to local space
            if (tc.HasParent())
            {
                Entity parent = entity.GetParent();
                if (parent && parent.HasComponent<TransformComponent>())
                {
                    Mat4 parentWorldMatrix = parent.GetComponent<TransformComponent>().WorldMatrix;
                    transformMatrix = inverse(parentWorldMatrix) * transformMatrix;
                }
            }

            // Only update the component relevant to the current operation
            // to avoid euler angle roundtrip instability
            if (m_GizmoOperation == ImGuizmo::TRANSLATE)
            {
                tc.Position = Vector3(transformMatrix[3]);
            }
            else if (m_GizmoOperation == ImGuizmo::ROTATE)
            {
                Vector3 decomposedScale, skew;
                Vector4 perspective;
                Quaternion rotation;
                Vector3 translation;
                decompose(transformMatrix, decomposedScale, rotation, translation, skew, perspective);
                tc.Rotation = eulerAngles(rotation);
            }
            else if (m_GizmoOperation == ImGuizmo::SCALE)
            {
                Vector3 decomposedScale, skew;
                Vector4 perspective;
                Quaternion rotation;
                Vector3 translation;
                decompose(transformMatrix, decomposedScale, rotation, translation, skew, perspective);
                tc.Scale = decomposedScale;
            }

            tc.Dirty = true;
        }
    }

    //--------------------------------------------------------------------------
    // Events
    //--------------------------------------------------------------------------

    void ViewportPanel::OnEvent(Event& e)
    {
        if (m_Hovered)
        {
            // Only block camera events when a gizmo is actually visible and hovered
            if (Selection::HasEntitySelected() && ImGuizmo::IsOver())
                return;

            m_CameraController.OnEvent(e);
        }
    }
}
