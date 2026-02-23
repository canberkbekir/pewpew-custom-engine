#include "GameViewportPanel.h"

#include <imgui.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include "CBEngine/Components/CameraComponent.h"
#include "CBEngine/Components/TransformComponent.h"
#include "CBEngine/Renderer/Core/RenderCommand.h"
#include "CBEngine/Scene/SceneManager.h"
#include "CBEngine/Systems/RendererSystem.h"

namespace CB
{
    GameViewportPanel::GameViewportPanel()
        : Panel("Game", true)
        , m_GameCamera(45.0f, 16.0f / 9.0f, 0.1f, 100.0f)
    {
        FramebufferSpecification fbSpec;
        fbSpec.Attachments = {
            {FramebufferTextureFormat::RGBA8},
            {FramebufferTextureFormat::Depth}
        };
        fbSpec.Width = 1280;
        fbSpec.Height = 720;
        m_Framebuffer = Framebuffer::Create(fbSpec);

        m_DefaultShader = Shader::Create("assets/shaders/PBR.glsl");

        m_DefaultMaterial = CreateRef<Material>();
        m_DefaultMaterial->SetAlbedo({0.8f, 0.8f, 0.8f});
        m_DefaultMaterial->SetRoughness(0.5f);
        m_DefaultMaterial->SetMetallic(0.0f);
    }

    void GameViewportPanel::OnUpdate(Timestep ts)
    {
        if (!m_Visible)
            return;

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
            }
        }

        // Find the primary camera entity
        m_HasActiveCamera = false;
        Ref<Scene> scene = SceneManager::GetActiveScene();
        if (scene)
        {
            auto view = scene->GetRegistry().view<TransformComponent, CameraComponent>();
            for (auto entity : view)
            {
                auto& cam = view.get<CameraComponent>(entity);
                if (!cam.Primary)
                    continue;

                auto& tc = view.get<TransformComponent>(entity);

                float aspect = m_ViewportSize.x / m_ViewportSize.y;
                m_GameCamera.SetProjection(cam.FOV, aspect, cam.NearClip, cam.FarClip);

                // Extract position from world transform
                Vector3 worldPos = tc.GetWorldPosition();
                m_GameCamera.SetPosition(worldPos);

                // Extract world-space forward from WorldMatrix so the full
                // parent hierarchy rotation (e.g. player yaw + camera pitch) is captured.
                Vector3 forward = glm::normalize(Vector3(tc.WorldMatrix[2]));

                float pitch = glm::degrees(asin(glm::clamp(forward.y, -1.0f, 1.0f)));
                float yaw = glm::degrees(atan2(forward.z, forward.x));
                m_GameCamera.SetRotation(pitch, yaw);

                m_HasActiveCamera = true;
                break;
            }
        }

        // Render
        m_Framebuffer->Bind();
        RenderScene();
        m_Framebuffer->Unbind();
    }

    void GameViewportPanel::RenderScene()
    {
        RenderCommand::SetClearColor({0.05f, 0.05f, 0.08f, 1.0f});
        RenderCommand::Clear();

        if (!m_HasActiveCamera)
            return;

        Ref<Scene> scene = SceneManager::GetActiveScene();
        RendererSystem::OnUpdate(
            scene.get(),
            m_GameCamera,
            m_GameCamera.GetPosition(),
            m_DefaultShader,
            m_DefaultMaterial
        );
    }

    void GameViewportPanel::OnImGuiRender()
    {
        if (!m_Visible)
            return;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::Begin(m_Name.c_str(), &m_Visible);

        ImVec2 availableSize = ImGui::GetContentRegionAvail();
        m_ViewportSize = {availableSize.x, availableSize.y};

        if (m_HasActiveCamera)
        {
            uint64_t textureID = m_Framebuffer->GetColorAttachmentRendererID();
            ImGui::Image(
                reinterpret_cast<void*>(textureID),
                availableSize,
                ImVec2(0, 1),
                ImVec2(1, 0)
            );
        }
        else
        {
            // Center the message
            ImVec2 textSize = ImGui::CalcTextSize("No active camera");
            ImVec2 cursorPos = ImGui::GetCursorPos();
            ImGui::SetCursorPos(ImVec2(
                cursorPos.x + (availableSize.x - textSize.x) * 0.5f,
                cursorPos.y + (availableSize.y - textSize.y) * 0.5f
            ));
            ImGui::TextDisabled("No active camera");
        }

        // Stats overlay
        if (m_ShowStats && m_HasActiveCamera)
        {
            ImVec2 windowPos = ImGui::GetWindowPos();
            ImVec2 contentMin = ImGui::GetWindowContentRegionMin();
            ImGui::SetNextWindowPos(ImVec2(
                windowPos.x + contentMin.x + 10.0f,
                windowPos.y + contentMin.y + 10.0f
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

            if (ImGui::Begin("##GameStats", nullptr, flags))
            {
                ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
                ImGui::Text("Size: %dx%d",
                            static_cast<int>(m_ViewportSize.x),
                            static_cast<int>(m_ViewportSize.y));
            }
            ImGui::End();
        }

        ImGui::End();
        ImGui::PopStyleVar();
    }
}
