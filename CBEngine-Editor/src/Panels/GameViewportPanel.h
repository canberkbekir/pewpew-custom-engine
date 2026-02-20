#pragma once

#include "Panel.h"
#include "CBEngine/Core/Core.h"
#include "CBEngine/Core/TimeStep.h"
#include "CBEngine/Math/CoreMath.h"
#include "CBEngine/Renderer/Core/Framebuffer.h"
#include "CBEngine/Renderer/Camera/PerspectiveCamera.h"
#include "CBEngine/Renderer/Resources/Shader.h"
#include "CBEngine/Renderer/Resources/Material.h"

namespace CB
{
    class GameViewportPanel : public Panel
    {
    public:
        GameViewportPanel();

        void OnUpdate(Timestep ts);
        void OnImGuiRender() override;

    private:
        void RenderScene();

        Ref<Framebuffer> m_Framebuffer;
        Ref<Shader> m_DefaultShader;
        Ref<Material> m_DefaultMaterial;

        PerspectiveCamera m_GameCamera;
        bool m_HasActiveCamera = false;

        Vector2 m_ViewportSize = {1280.0f, 720.0f};
        bool m_ShowStats = false;
    };
}
