#include "ViewportPanel.h"

#include <imgui.h>
#include <imgui_internal.h>

#include "CBEngine/Debug/Instrumentor.h"
#include "CBEngine/Renderer/Core/RenderCommand.h"
#include "CBEngine/Renderer/Core/Renderer3D.h"
#include "CBEngine/Scene/SceneManager.h"
#include "CBEngine/Scene/Entity.h"
#include "CBEngine/Components/Components.h"

namespace CB
{
    //--------------------------------------------------------------------------
    // Construction
    //--------------------------------------------------------------------------

    ViewportPanel::ViewportPanel()
        : Panel("Viewport", true)
        , m_CameraController(45.0f, 1280.0f / 720.0f, 0.1f, 100.0f)
    {
        // Create framebuffer
        FramebufferSpecification fbSpec;
        fbSpec.Width = 1280;
        fbSpec.Height = 720;
        m_Framebuffer = Framebuffer::Create(fbSpec);

        // Load default shader
        m_DefaultShader = Shader::Create("assets/shaders/PBR.glsl");

        // Create default material
        m_DefaultMaterial = CreateRef<Material>();
        m_DefaultMaterial->SetAlbedo({ 0.8f, 0.8f, 0.8f });
        m_DefaultMaterial->SetRoughness(0.5f);
        m_DefaultMaterial->SetMetallic(0.0f);

        // Set up lighting
        Renderer3D::SetDirectionalLight(m_LightDirection, m_LightColor, m_LightIntensity);
        Renderer3D::SetAmbientLight(m_AmbientColor);
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

        // Update camera only when focused
        if (m_Focused)
            m_CameraController.OnUpdate(ts);

        // Render to framebuffer
        m_Framebuffer->Bind();
        RenderScene();
        m_Framebuffer->Unbind();
    }

    void ViewportPanel::RenderScene()
    {
        // Clear
        RenderCommand::SetClearColor({ 0.15f, 0.15f, 0.18f, 1.0f });
        RenderCommand::Clear();

        // Wireframe mode
        RenderCommand::SetWireframeMode(m_Wireframe);

        // Begin scene
        Renderer3D::BeginScene(
            m_CameraController.GetCamera(),
            m_CameraController.GetCamera().GetPosition()
        );

        // Render entities
        Ref<Scene> scene = SceneManager::GetActiveScene();
        if (scene)
        {
            // Render MeshRendererComponent entities
            {
                auto view = scene->GetRegistry().view<TransformComponent, MeshRendererComponent>();
                for (auto entityID : view)
                {
                    auto& transform = view.get<TransformComponent>(entityID);
                    auto& meshRenderer = view.get<MeshRendererComponent>(entityID);

                    if (!meshRenderer.Visible || !meshRenderer.MeshAsset)
                        continue;

                    Ref<Shader> shader = meshRenderer.ShaderAsset
                        ? meshRenderer.ShaderAsset
                        : m_DefaultShader;

                    Ref<Material> material = meshRenderer.MaterialAsset
                        ? meshRenderer.MaterialAsset
                        : m_DefaultMaterial;

                    Renderer3D::Submit(shader, material, meshRenderer.MeshAsset, transform.GetTransform());
                }
            }

            // Render VoxelRendererComponent entities
            {
                auto view = scene->GetRegistry().view<TransformComponent, VoxelRendererComponent>();
                for (auto entityID : view)
                {
                    auto& transform = view.get<TransformComponent>(entityID);
                    auto& voxelRenderer = view.get<VoxelRendererComponent>(entityID);

                    if (!voxelRenderer.Visible || !voxelRenderer.MeshAsset)
                        continue;

                    Ref<Shader> shader = voxelRenderer.ShaderAsset
                        ? voxelRenderer.ShaderAsset
                        : m_DefaultShader;

                    Ref<Material> material = voxelRenderer.MaterialAsset
                        ? voxelRenderer.MaterialAsset
                        : m_DefaultMaterial;

                    Renderer3D::Submit(shader, material, voxelRenderer.MeshAsset, transform.GetTransform());
                }
            }
        }

        Renderer3D::EndScene();

        // Restore fill mode
        RenderCommand::SetWireframeMode(false);
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
        m_ViewportSize = { availableSize.x, availableSize.y };

        // Display framebuffer texture (UV flipped for OpenGL)
        uint64_t textureID = m_Framebuffer->GetColorAttachmentRendererID();
        ImGui::Image(
            reinterpret_cast<void*>(textureID),
            availableSize,
            ImVec2(0, 1),
            ImVec2(1, 0)
        );

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
        if (ImGui::Selectable("Stats", m_ShowStats, 0, ImVec2(40, 0)))
            m_ShowStats = !m_ShowStats;

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
    // Events
    //--------------------------------------------------------------------------

    void ViewportPanel::OnEvent(Event& e)
    {
        if (m_Hovered)
            m_CameraController.OnEvent(e);
    }
}