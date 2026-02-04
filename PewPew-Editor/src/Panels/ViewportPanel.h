#pragma once

#include "Panel.h"
#include "PewPew/Core/Core.h"
#include "PewPew/Math/CoreMath.h"
#include "PewPew/Renderer/Core/Framebuffer.h"
#include "PewPew/Renderer/Camera/PerspectiveCameraController.h"
#include "PewPew/Renderer/Resources/Mesh.h"
#include "PewPew/Renderer/Resources/Shader.h"
#include "PewPew/Renderer/Resources/Material.h"

namespace PewPew
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

	private:
		// Rendering
		Ref<Framebuffer> m_Framebuffer;
		Ref<Shader> m_DefaultShader;
		Ref<Material> m_DefaultMaterial;

		// Camera
		PerspectiveCameraController m_CameraController;

		// Viewport state
		Vector2 m_ViewportSize = { 1280.0f, 720.0f };
		bool m_Focused = false;
		bool m_Hovered = false;

		// Toolbar options
		bool m_Wireframe = false;
		bool m_ShowStats = true;

		// Lighting
		Vector3 m_LightDirection = { -0.2f, -1.0f, -0.3f };
		Vector3 m_LightColor = { 1.0f, 1.0f, 1.0f };
		float m_LightIntensity = 1.0f;
		Vector3 m_AmbientColor = { 0.1f, 0.1f, 0.1f };
	};
}