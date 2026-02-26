#include "cbpch.h"
#include "CBEngine/Renderer/Core/Renderer.h"

#include "Renderer3D.h"
#include "CBEngine/Renderer/Core/RenderCommand.h"
#include "CBEngine/Renderer/Core/ShaderUniforms.h"
#include "CBEngine/Debug/Instrumentor.h"

namespace CB
{
	Scope<Renderer::SceneData> Renderer::s_SceneData = CreateScope<SceneData>();

	void Renderer::Init()
	{
		CB_PROFILE_FUNCTION();
		RenderCommand::Init();
	}

	void Renderer::Shutdown()
	{
		CB_PROFILE_FUNCTION();
		Renderer3D::Shutdown();
	}

	void Renderer::OnWindowResize(uint32_t width,uint32_t height)
	{
		CB_PROFILE_FUNCTION();
		RenderCommand::SetViewport(0, 0, width, height);
	}

	void Renderer::BeginScene(const Camera& camera)
	{
		CB_PROFILE_FUNCTION();
		s_SceneData->ViewProjectionMatrix = camera.GetViewProjectionMatrix();
	}

	void Renderer::EndScene() { CB_PROFILE_FUNCTION(); }

	void Renderer::Submit(const Ref<Shader>& shader,const Ref<VertexArray>& vertexArray,const Mat4& transform)
	{
		CB_PROFILE_FUNCTION();
		using namespace ShaderUniforms;

		shader->Bind();
		shader->SetMat4(ViewProjection, s_SceneData->ViewProjectionMatrix);
		shader->SetMat4(Transform, transform);

		vertexArray->Bind();
		RenderCommand::DrawIndexed(vertexArray);
	}
}