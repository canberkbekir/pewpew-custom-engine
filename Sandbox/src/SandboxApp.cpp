#include <PewPew.h>

#include "imgui/imgui.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Platform/OpenGL/OpenGLShader.h"
 
class Layer3D : public PewPew::Layer
{
public:
	Layer3D()
		: Layer("ExampleLayer"), m_Camera(45.0f, 1280.0f / 720.0f, 0.1f, 100.0f)
	{
		// Position camera to view the mesh
		m_CameraPosition = { 0.0f, 1.0f, 4.0f };
		m_Camera.SetPosition(m_CameraPosition);
 
		m_Mesh = PewPew::Mesh::Load("assets/models/Gelatinous_Cube.fbx");

		// Load shader and texture
		m_Shader = PewPew::Shader::Create("assets/shaders/Basic3D.glsl");
		m_Texture = PewPew::Texture2D::Create("assets/textures/Texture.png");

		std::dynamic_pointer_cast<PewPew::OpenGLShader>(m_Shader)->Bind();
		std::dynamic_pointer_cast<PewPew::OpenGLShader>(m_Shader)->UploadUniformInt("u_Texture", 0);
	}

	void OnUpdate(PewPew::Timestep ts) override
	{
		// Camera controls
		float cameraSpeed = 3.0f * ts;

		if (PewPew::Input::IsKeyPressed(PEW_KEY_W))
			m_CameraPosition += m_Camera.GetForwardDirection() * cameraSpeed;
		if (PewPew::Input::IsKeyPressed(PEW_KEY_S))
			m_CameraPosition -= m_Camera.GetForwardDirection() * cameraSpeed;
		if (PewPew::Input::IsKeyPressed(PEW_KEY_A))
			m_CameraPosition -= m_Camera.GetRightDirection() * cameraSpeed;
		if (PewPew::Input::IsKeyPressed(PEW_KEY_D))
			m_CameraPosition += m_Camera.GetRightDirection() * cameraSpeed;
		if (PewPew::Input::IsKeyPressed(PEW_KEY_SPACE))
			m_CameraPosition.y += cameraSpeed;
		if (PewPew::Input::IsKeyPressed(PEW_KEY_LEFT_SHIFT))
			m_CameraPosition.y -= cameraSpeed;

		// Camera rotation with arrow keys
		float rotSpeed = 60.0f * ts;
		if (PewPew::Input::IsKeyPressed(PEW_KEY_LEFT))
			m_CameraYaw -= rotSpeed;
		if (PewPew::Input::IsKeyPressed(PEW_KEY_RIGHT))
			m_CameraYaw += rotSpeed;
		if (PewPew::Input::IsKeyPressed(PEW_KEY_UP))
			m_CameraPitch += rotSpeed;
		if (PewPew::Input::IsKeyPressed(PEW_KEY_DOWN))
			m_CameraPitch -= rotSpeed;

		// Clamp pitch to avoid gimbal lock
		m_CameraPitch = glm::clamp(m_CameraPitch, -89.0f, 89.0f);

		m_Camera.SetPosition(m_CameraPosition);
		m_Camera.SetRotation(m_CameraPitch, m_CameraYaw);

		// Update mesh rotation
		m_MeshRotation += m_RotationSpeed * ts;

		// Render
		PewPew::RenderCommand::SetClearColor({ 0.1f, 0.1f, 0.15f, 1.0f });
		PewPew::RenderCommand::Clear();

		PewPew::Renderer::BeginScene(m_Camera);

		// Only render if mesh loaded successfully
		if (m_Mesh)
		{
			Mat4 rotation = glm::rotate(Mat4(1.0f), glm::radians(m_MeshRotation), glm::normalize(Vector3(0.5f, 1.0f, 0.0f)));
			Mat4 transform = rotation;

			m_Texture->Bind();
			m_Shader->Bind();
			std::dynamic_pointer_cast<PewPew::OpenGLShader>(m_Shader)->UploadUniformMat4("u_ViewProjection", m_Camera.GetViewProjectionMatrix());
			std::dynamic_pointer_cast<PewPew::OpenGLShader>(m_Shader)->UploadUniformMat4("u_Transform", transform);

			m_Mesh->Bind();
			PewPew::RenderCommand::DrawIndexed(m_Mesh->GetVertexArray());
		}

		PewPew::Renderer::EndScene();
	}

	void OnImGuiRender() override
	{
		ImGui::Begin("3D Controls");
		ImGui::Text("Camera Position: (%.2f, %.2f, %.2f)", m_CameraPosition.x, m_CameraPosition.y, m_CameraPosition.z);
		ImGui::Text("Camera Pitch: %.2f, Yaw: %.2f", m_CameraPitch, m_CameraYaw);
		ImGui::Separator();

		if (m_Mesh)
		{
			ImGui::Text("Mesh loaded: %u indices", m_Mesh->GetIndexCount());
			ImGui::SliderFloat("Rotation Speed", &m_RotationSpeed, 0.0f, 180.0f);
		}
		else
		{
			ImGui::TextColored(ImVec4(1, 0, 0, 1), "Mesh not loaded!");
			ImGui::Text("Add cube.obj to: assets/models/cube.obj");
		}

		ImGui::Separator();
		ImGui::Text("Controls:");
		ImGui::Text("  WASD - Move camera");
		ImGui::Text("  Space/Shift - Up/Down");
		ImGui::Text("  Arrow Keys - Look around");
		ImGui::End();
	}

	void OnEvent(PewPew::Event& event) override {}

private:
	PewPew::PerspectiveCamera m_Camera;
	Vector3 m_CameraPosition;
	float m_CameraPitch = 0.0f;
	float m_CameraYaw = -90.0f;

	PewPew::Ref<PewPew::Mesh> m_Mesh;
	PewPew::Ref<PewPew::Shader> m_Shader;
	PewPew::Ref<PewPew::Texture2D> m_Texture;

	float m_MeshRotation = 0.0f;
	float m_RotationSpeed = 45.0f;
};

// ============================================================================
// Sandbox Application
// ============================================================================
class Sandbox : public PewPew::Application
{
public:
	Sandbox()
	{
		PushLayer(new Layer3D());
	}

	~Sandbox() {}
};

PewPew::Application* PewPew::CreateApplication()
{
	return new Sandbox();
}
