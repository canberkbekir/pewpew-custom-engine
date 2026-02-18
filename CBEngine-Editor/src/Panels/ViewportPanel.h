#pragma once

#include "Panel.h"
#include "CBEngine/Core/Core.h"
#include "CBEngine/Math/CoreMath.h"
#include "CBEngine/Scene/Entity.h"
#include "CBEngine/Renderer/Core/Framebuffer.h"
#include "CBEngine/Renderer/Camera/PerspectiveCameraController.h"
#include "CBEngine/Renderer/Resources/Mesh.h"
#include "CBEngine/Renderer/Resources/Shader.h"
#include "CBEngine/Renderer/Resources/Material.h"
#include <imgui.h>
#include <ImGuizmo/ImGuizmo.h>

namespace CB
{
    class ViewportPanel : public Panel
    {
    public:
        ViewportPanel();

        void OnUpdate(Timestep ts);
        void OnImGuiRender() override;
        void OnEvent(Event& e) override;

    private:
        void RenderToolbar();
        void RenderStatsOverlay();
        void RenderScene();
        void RenderGizmo();

        void HandleEntityPicking();
        void DrawEntityColliders(Entity entity, bool recurseChildren = true);

        // Rendering
        Ref<Framebuffer> m_Framebuffer;
        Ref<Shader> m_DefaultShader;
        Ref<Material> m_DefaultMaterial;

        // Camera
        PerspectiveCameraController m_CameraController;

        // Viewport state
        Vector2 m_ViewportSize = {1280.0f, 720.0f};
        bool m_Focused = false;
        bool m_Hovered = false;

        // Toolbar options
        bool m_Wireframe = false;
        bool m_ShowColliders = false;
        bool m_ShowStats = true;

        // Gizmo state
        ImGuizmo::OPERATION m_GizmoOperation = ImGuizmo::TRANSLATE;
        ImGuizmo::MODE m_GizmoMode = ImGuizmo::LOCAL;
    };
}
