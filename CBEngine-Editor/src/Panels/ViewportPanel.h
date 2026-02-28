#pragma once

#include "Panel.h"
#include "CBEngine/Core/Core.h"
#include "CBEngine/Math/CoreMath.h"
#include "CBEngine/Scene/Entity.h"
#include "CBEngine/Renderer/Core/Framebuffer.h"
#include "CBEngine/Renderer/Camera/PerspectiveCameraController.h"
#include "CBEngine/Renderer/Camera/PerspectiveCamera.h"
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
		PerspectiveCameraController& GetCameraController() { return m_CameraController; }

	private:
		void RenderToolbar();
		void RenderStatsOverlay();
		bool FindPrimarySceneCamera();
		void RenderScene();
		void RenderGizmo();

		void HandleEntityPicking();
		void DrawEntityColliders(Entity entity,bool recurseChildren = true);
		void DrawHeatView();

		// Rendering
		Ref<Framebuffer> m_Framebuffer;
		Ref<Shader> m_DefaultShader;
		Ref<Material> m_DefaultMaterial;

		// Camera
		PerspectiveCameraController m_CameraController;
		PerspectiveCamera m_PlayCamera;
		bool m_HasPlayCamera = false;

		// Viewport state
		Vector2 m_ViewportSize = {1280.0f, 720.0f};
		bool m_Focused = false;
		bool m_Hovered = false;

		// Toolbar options
		bool m_Wireframe = false;
		bool m_ShowColliders = false;
		bool m_ShowStats = true;
		bool m_ShowGrid = true;
		bool m_ShowCameras = true;
		bool m_ShowHeatView = false;

		// Gizmo state
		ImGuizmo::OPERATION m_GizmoOperation = ImGuizmo::TRANSLATE;
		ImGuizmo::MODE m_GizmoMode = ImGuizmo::LOCAL;

		//Icons
		ImVec2 m_IconSize = {20.f, 20.f};
		Ref<Texture2D> m_WireframeButtonIcon;

	};
}