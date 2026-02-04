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

		void OnImGuiRender() override;
		void OnUpdate(Timestep ts);
		void OnEvent(Event& e) override;

		bool IsFocused() const { return m_Focused; }
		bool IsHovered() const { return m_Hovered; }

		PerspectiveCameraController& GetCameraController() { return m_CameraController; }
	private:
		Ref<Framebuffer> m_Framebuffer;
		PerspectiveCameraController m_CameraController;

		Vector2 m_ViewportSize = {0.0f, 0.0f};
		bool m_Focused = false;
		bool m_Hovered = false;

		// Default resources for entities without custom shader/material
		Ref<Shader> m_DefaultShader;
		Ref<Material> m_DefaultMaterial;

		// Lighting
		Vector3 m_LightDirection = {-0.5f, -1.0f, -0.3f};
		Vector3 m_LightColor = {1.0f, 0.98f, 0.95f};
		float m_LightIntensity = 2.0f;
		Vector3 m_AmbientColor = {0.03f, 0.03f, 0.05f};
	};
}