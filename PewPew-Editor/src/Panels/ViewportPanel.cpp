#include "ViewportPanel.h"

#include "imgui.h"
#include "glm/ext/matrix_transform.hpp"
#include "PewPew/Debug/Instrumentor.h"
#include "PewPew/Renderer/Core/RenderCommand.h"
#include "PewPew/Renderer/Core/Renderer3D.h"
#include "PewPew/Renderer/Resources/Texture.h"

namespace PewPew
{
	ViewportPanel::ViewportPanel()
		: Panel("Viewport", true),
		  m_CameraController(45.0f, 1280.0f / 720.0f, 0.1f, 100.0f)
	{
		// Create framebuffer
		FramebufferSpecification fbSpec;
		fbSpec.Width = 1280;
		fbSpec.Height = 720;
		m_Framebuffer = Framebuffer::Create(fbSpec);

		// Load mesh
		m_Mesh = Mesh::Load("assets/models/Matilda.fbx");

		// Load shader
		m_Shader = Shader::Create("assets/shaders/PBR.glsl");

		// Create material with texture
		m_Material = CreateRef<Material>();
		m_Material->SetAlbedoMap(Texture2D::Create("assets/textures/MatildaTexture.png"));
		m_Material->SetRoughness(0.5f);
		m_Material->SetMetallic(0.0f);

		// Set up lighting
		Renderer3D::SetDirectionalLight(m_LightDirection, m_LightColor, m_LightIntensity);
		Renderer3D::SetAmbientLight(m_AmbientColor);
	}

	void ViewportPanel::OnUpdate(Timestep ts)
	{
		PEW_PROFILE_FUNCTION();

		// Resize framebuffer if needed
		FramebufferSpecification spec = m_Framebuffer->GetSpecification();
		if (m_ViewportSize.x > 0.0f && m_ViewportSize.y > 0.0f &&
			(spec.Width != static_cast<uint32_t>(m_ViewportSize.x) ||
				spec.Height != static_cast<uint32_t>(m_ViewportSize.y))) {
			m_Framebuffer->Resize(static_cast<uint32_t>(m_ViewportSize.x), static_cast<uint32_t>(m_ViewportSize.y));
			m_CameraController.SetViewportSize(m_ViewportSize.x, m_ViewportSize.y);
		}

		// Only update camera if viewport is focused
		if (m_Focused) { m_CameraController.OnUpdate(ts); }

		// Update rotation
		m_MeshRotation += m_RotationSpeed * ts;

		// Bind framebuffer - render to texture
		m_Framebuffer->Bind();

		// Clear framebuffer
		RenderCommand::SetClearColor({0.15f, 0.15f, 0.18f, 1.0f});
		RenderCommand::Clear();

		// Render scene
		Renderer3D::BeginScene(m_CameraController.GetCamera(), m_CameraController.GetCamera().GetPosition());

		// Render mesh
		if (m_Mesh) {
			Mat4 transform = glm::scale(Mat4(1.0f), Vector3(m_MeshScale));
			transform = glm::rotate(transform, glm::radians(m_MeshRotation), Vector3(0.0f, 1.0f, 0.0f));

			Renderer3D::Submit(m_Shader, m_Material, m_Mesh, transform);
		}

		Renderer3D::EndScene();

		// Unbind framebuffer
		m_Framebuffer->Unbind();
	}

	void ViewportPanel::OnImGuiRender()
	{
		if (!m_Visible)
			return;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::Begin(m_Name.c_str(), &m_Visible);

		// Check if viewport is focused/hovered for input
		m_Focused = ImGui::IsWindowFocused();
		m_Hovered = ImGui::IsWindowHovered();

		// Get viewport size for framebuffer rendering
		ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
		m_ViewportSize = {viewportPanelSize.x, viewportPanelSize.y};

		// Display framebuffer texture
		uint64_t textureID = m_Framebuffer->GetColorAttachmentRendererID();
		ImGui::Image(
			reinterpret_cast<void*>(textureID),
			ImVec2{m_ViewportSize.x, m_ViewportSize.y},
			ImVec2{0, 1}, // UV min (flipped for OpenGL)
			ImVec2{1, 0} // UV max (flipped for OpenGL)
		);

		ImGui::End();
		ImGui::PopStyleVar();
	}

	void ViewportPanel::OnEvent(Event& e) { m_CameraController.OnEvent(e); }
}