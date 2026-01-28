#include "pewpch.h"
#include "PewPew/Renderer/Core/Renderer.h"

#include "PewPew/Renderer/Core/RenderCommand.h"
#include "Platform/OpenGL/OpenGLShader.h"

namespace PewPew {

	Renderer::SceneData* Renderer::s_SceneData = new Renderer::SceneData;

	void Renderer::Init()
	{
		RenderCommand::Init();
	}

	void Renderer::OnWindowResize(uint32_t width, uint32_t height)
	{
		RenderCommand::SetViewport(0, 0, width, height);
	}

	void Renderer::BeginScene(const Camera& camera)
	{
		s_SceneData->ViewProjectionMatrix = camera.GetViewProjectionMatrix();
	}

	void Renderer::BeginScene(const Camera& camera, const Vector3& cameraPosition)
	{
		s_SceneData->ViewProjectionMatrix = camera.GetViewProjectionMatrix();
		s_SceneData->CameraPosition = cameraPosition;
	}

	void Renderer::EndScene()
	{
	}

	void Renderer::SetDirectionalLight(const Vector3& direction, const Vector3& color, float intensity)
	{
		s_SceneData->LightDirection = direction;
		s_SceneData->LightColor = color;
		s_SceneData->LightIntensity = intensity;
	}

	void Renderer::SetAmbientLight(const Vector3& color)
	{
		s_SceneData->AmbientColor = color;
	}

	void Renderer::Submit(const std::shared_ptr<Shader>& shader, const std::shared_ptr<VertexArray>& vertexArray, const Mat4& transform)
	{
		shader->Bind();
		std::dynamic_pointer_cast<OpenGLShader>(shader)->UploadUniformMat4("u_ViewProjection", s_SceneData->ViewProjectionMatrix);
		std::dynamic_pointer_cast<OpenGLShader>(shader)->UploadUniformMat4("u_Transform", transform);

		vertexArray->Bind();
		RenderCommand::DrawIndexed(vertexArray);
	}

	void Renderer::Submit(const Ref<Shader>& shader, const Ref<Material>& material, const Ref<Mesh>& mesh, const Mat4& transform)
	{
		shader->Bind();

		// Upload transform uniforms
		shader->SetMat4("u_ViewProjection", s_SceneData->ViewProjectionMatrix);
		shader->SetMat4("u_Transform", transform);
		shader->SetFloat3("u_CameraPosition", s_SceneData->CameraPosition);

		// Upload light uniforms
		shader->SetFloat3("u_LightDirection", s_SceneData->LightDirection);
		shader->SetFloat3("u_LightColor", s_SceneData->LightColor);
		shader->SetFloat("u_LightIntensity", s_SceneData->LightIntensity);
		shader->SetFloat3("u_AmbientColor", s_SceneData->AmbientColor);

		// Bind material (textures and properties)
		material->Bind(shader);

		// Draw mesh
		mesh->Bind();
		RenderCommand::DrawIndexed(mesh->GetVertexArray());
	}

}
