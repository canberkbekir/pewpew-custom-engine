#include <PewPew.h>

#include "imgui/imgui.h"

#include <glm/gtc/matrix_transform.hpp>

#include "glm/gtc/type_ptr.hpp"
#include "PewPew/Core/String.h"
#include "Platform/OpenGL/OpenGLShader.h"

class ExampleLayer : public PewPew::Layer
{
public:
	ExampleLayer()
		: Layer("Example"), m_Camera(-1.6f, 1.6f, -0.9f, 0.9f), m_CameraPosition(0.0f)
	{
		m_VertexArray.reset(PewPew::VertexArray::Create());

		float vertices[3 * 7] = {
			-0.5f, -0.5f, 0.0f, 0.8f, 0.2f, 0.8f, 1.0f,
			 0.5f, -0.5f, 0.0f, 0.2f, 0.3f, 0.8f, 1.0f,
			 0.0f,  0.5f, 0.0f, 0.8f, 0.8f, 0.2f, 1.0f
		};

		PewPew::Ref<PewPew::VertexBuffer> vertexBuffer;
		vertexBuffer.reset(PewPew::VertexBuffer::Create(vertices, sizeof(vertices)));
		PewPew::BufferLayout layout = {
			{ PewPew::ShaderDataType::Float3, "a_Position" },
			{ PewPew::ShaderDataType::Float4, "a_Color" }
		};
		vertexBuffer->SetLayout(layout);
		m_VertexArray->AddVertexBuffer(vertexBuffer);

		uint32_t indices[3] = { 0, 1, 2 };
		PewPew::Ref<PewPew::IndexBuffer> indexBuffer;
		indexBuffer.reset(PewPew::IndexBuffer::Create(indices, sizeof(indices) / sizeof(uint32_t)));
		m_VertexArray->SetIndexBuffer(indexBuffer);

		m_SquareVA.reset(PewPew::VertexArray::Create());

		float squareVertices[5 * 4] = {
			-0.5f, -0.5f, 0.0f, 0.0f, 0.0f,
			 0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
			 0.5f,  0.5f, 0.0f, 1.0f, 1.0f,
			-0.5f,  0.5f, 0.0f, 0.0f, 1.0f,
		};

		PewPew::Ref<PewPew::VertexBuffer> squareVB;
		squareVB.reset(PewPew::VertexBuffer::Create(squareVertices, sizeof(squareVertices)));
		squareVB->SetLayout({
			{ PewPew::ShaderDataType::Float3, "a_Position" },
            { PewPew::ShaderDataType::Float2, "a_TexCoord" }
		});
		m_SquareVA->AddVertexBuffer(squareVB);

		uint32_t squareIndices[6] = { 0, 1, 2, 2, 3, 0 };
		PewPew::Ref<PewPew::IndexBuffer> squareIB;
		squareIB.reset(PewPew::IndexBuffer::Create(squareIndices, sizeof(squareIndices) / sizeof(uint32_t)));
		m_SquareVA->SetIndexBuffer(squareIB);

		PewPew::String vertexSrc = R"(
			#version 330 core
			
			layout(location = 0) in vec3 a_Position;
			layout(location = 1) in vec4 a_Color;

			uniform mat4 u_ViewProjection;
			uniform mat4 u_Transform;

			out vec3 v_Position;
			out vec4 v_Color;

			void main()
			{
				v_Position = a_Position;
				v_Color = a_Color;
				gl_Position = u_ViewProjection * u_Transform * vec4(a_Position, 1.0);	
			}
		)";

		PewPew::String fragmentSrc = R"(
			#version 330 core
			
			layout(location = 0) out vec4 color;

			in vec3 v_Position;
			in vec4 v_Color;

			void main()
			{
				color = vec4(v_Position * 0.5 + 0.5, 1.0);
				color = v_Color;
			}
		)";

		m_Shader.reset(PewPew::Shader::Create(vertexSrc, fragmentSrc));
		
		PewPew::String flatColorShaderVertexSrc  = R"(
			#version 330 core
			
			layout(location = 0) in vec3 a_Position;

			uniform mat4 u_ViewProjection;
			uniform mat4 u_Transform;

			out vec3 v_Position;

			void main()
			{
				v_Position = a_Position;
				gl_Position = u_ViewProjection * u_Transform * vec4(a_Position, 1.0);	
			}
		)";

		PewPew::String flatColorShaderFragmentSrc  = R"(
			#version 330 core
			
			layout(location = 0) out vec4 color;

			in vec3 v_Position;
			uniform vec3 u_Color;
			void main()
			{
				color = vec4(u_Color, 1.0);
			}
		)";

		m_FlatColorShader.reset(PewPew::Shader::Create(flatColorShaderVertexSrc, flatColorShaderFragmentSrc));
		
		m_TextureShader.reset(PewPew::Shader::Create("assets/shaders/Texture.glsl"));
		
		m_Texture = PewPew::Texture2D::Create("assets/textures/Texture.png");
		m_IconTexture = PewPew::Texture2D::Create("assets/textures/Icon.png");

		std::dynamic_pointer_cast<PewPew::OpenGLShader>(m_TextureShader)->Bind();
		std::dynamic_pointer_cast<PewPew::OpenGLShader>(m_TextureShader)->UploadUniformInt("u_Texture", 0);

	}

	void OnUpdate(PewPew::Timestep ts) override
	{
		if (PewPew::Input::IsKeyPressed(PEW_KEY_LEFT))
			m_CameraPosition.x -= m_CameraMoveSpeed * ts;
		else if (PewPew::Input::IsKeyPressed(PEW_KEY_RIGHT))
			m_CameraPosition.x += m_CameraMoveSpeed * ts;

		if (PewPew::Input::IsKeyPressed(PEW_KEY_UP))
			m_CameraPosition.y += m_CameraMoveSpeed * ts;
		else if (PewPew::Input::IsKeyPressed(PEW_KEY_DOWN))
			m_CameraPosition.y -= m_CameraMoveSpeed * ts;

		if (PewPew::Input::IsKeyPressed(PEW_KEY_A))
			m_CameraRotation += m_CameraRotationSpeed * ts;
		if (PewPew::Input::IsKeyPressed(PEW_KEY_D))
			m_CameraRotation -= m_CameraRotationSpeed * ts;

		PewPew::RenderCommand::SetClearColor({ 0.1f, 0.1f, 0.1f, 1 });
		PewPew::RenderCommand::Clear();

		m_Camera.SetPosition(m_CameraPosition);
		m_Camera.SetRotation(m_CameraRotation);

		PewPew::Renderer::BeginScene(m_Camera);

		Mat4 scale = glm::scale(Mat4(1.0f), Vector3(0.1f));
		std::dynamic_pointer_cast<PewPew::OpenGLShader>(m_FlatColorShader)->Bind();
		std::dynamic_pointer_cast<PewPew::OpenGLShader>(m_FlatColorShader)->UploadUniformFloat3("u_Color", m_SquareColor);
		for (int y = 0; y < 20; y++)
		{
			for (int x = 0; x < 20; x++)
			{
				Vector3 pos(x * 0.11f, y * 0.11f, 0.0f);
				Mat4 transform = glm::translate(Mat4(1.0f), pos) * scale;
				PewPew::Renderer::Submit(m_FlatColorShader, m_SquareVA, transform);
			}
		}

		m_Texture->Bind();
		PewPew::Renderer::Submit(m_TextureShader, m_SquareVA, glm::scale(glm::mat4(1.0f), glm::vec3(1.5f)));

		m_IconTexture->Bind();
		PewPew::Renderer::Submit(m_TextureShader, m_SquareVA, glm::scale(glm::mat4(1.0f), glm::vec3(1.5f)));

		// Triangle
		// Hazel::Renderer::Submit(m_Shader, m_VertexArray);

		PewPew::Renderer::EndScene();
	}

	virtual void OnImGuiRender() override
	{
		ImGui::Begin("Settings");
		ImGui::ColorEdit3("Square Color", glm::value_ptr(m_SquareColor));
		ImGui::End();
	}

	void OnEvent(PewPew::Event& event) override
	{
	}
private:
	PewPew::Ref<PewPew::Shader> m_Shader;
	PewPew::Ref<PewPew::VertexArray> m_VertexArray;

	PewPew::Ref<PewPew::Shader> m_FlatColorShader,m_TextureShader;
	PewPew::Ref<PewPew::VertexArray> m_SquareVA;

	PewPew::Ref<PewPew::Texture2D> m_Texture,m_IconTexture;

	PewPew::OrthographicCamera m_Camera;
	Vector3 m_CameraPosition;
	float m_CameraMoveSpeed = 5.0f;

	float m_CameraRotation = 0.0f;
	float m_CameraRotationSpeed = 180.0f;

	Vector3 m_SquareColor = { 0.2f, 0.3f, 0.8f };
};

class Sandbox : public PewPew::Application
{
public:
	Sandbox()
	{
		PushLayer(new ExampleLayer());
	}

	~Sandbox()
	{

	}

};

PewPew::Application* PewPew::CreateApplication()
{
	return new Sandbox();
}