#include <PewPew.h>

#include "imgui/imgui.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "PewPew/Renderer/PerspectiveCameraController.h"
#include "Platform/OpenGL/OpenGLShader.h"
 
class Layer3D : public PewPew::Layer
{
public:
	Layer3D()
		: Layer("ExampleLayer"), m_CameraController(45.0f, 1280.0f / 720.0f, 0.1f, 100.0f)
	{
		  
		m_Mesh = PewPew::Mesh::Load("assets/models/Gelatinous_Cube.fbx");

		// Load shader and texture
		m_Shader = PewPew::Shader::Create("assets/shaders/Basic3D.glsl");
		m_Texture = PewPew::Texture2D::Create("assets/textures/Texture.png");

		std::dynamic_pointer_cast<PewPew::OpenGLShader>(m_Shader)->Bind();
		std::dynamic_pointer_cast<PewPew::OpenGLShader>(m_Shader)->UploadUniformInt("u_Texture", 0);
	}

	void OnUpdate(PewPew::Timestep ts) override
	{
		 
		m_CameraController.OnUpdate(ts);
		// Update mesh rotation
		m_MeshRotation += m_RotationSpeed * ts;

		// Render
		PewPew::RenderCommand::SetClearColor({ 0.1f, 0.1f, 0.15f, 1.0f });
		PewPew::RenderCommand::Clear();

		PewPew::Renderer::BeginScene(m_CameraController.GetCamera());

		// Only render if mesh loaded successfully
		if (m_Mesh)
		{
			Mat4 rotation = glm::rotate(Mat4(1.0f), glm::radians(m_MeshRotation), glm::normalize(Vector3(0.5f, 1.0f, 0.0f)));
			Mat4 transform = rotation;

			m_Texture->Bind();
			m_Shader->Bind();
			std::dynamic_pointer_cast<PewPew::OpenGLShader>(m_Shader)->UploadUniformMat4("u_ViewProjection", m_CameraController.GetCamera().GetViewProjectionMatrix());
			std::dynamic_pointer_cast<PewPew::OpenGLShader>(m_Shader)->UploadUniformMat4("u_Transform", transform);

			m_Mesh->Bind();
			PewPew::RenderCommand::DrawIndexed(m_Mesh->GetVertexArray());
		}

		PewPew::Renderer::EndScene();
	}

	void OnImGuiRender() override
	{
		ImGui::Begin("3D Controls"); 
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

		/*ImGui::Separator();
		ImGui::Text("Controls:");
		ImGui::Text("  WASD - Move camera");
		ImGui::Text("  Space/Shift - Up/Down");
		ImGui::Text("  Arrow Keys - Look around");*/
		ImGui::End();
	}

	void OnEvent(PewPew::Event& event) override
	{
		m_CameraController.OnEvent(event);
	}

private:
	 PewPew::PerspectiveCameraController m_CameraController;

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
