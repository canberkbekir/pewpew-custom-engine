#include "pewpch.h"
#include "PewPew/Renderer/Core/Renderer.h"

#include "Renderer3D.h"
#include "PewPew/Renderer/Core/RenderCommand.h"
#include "PewPew/Renderer/Core/ShaderUniforms.h"

namespace PewPew
{
    Scope<Renderer::SceneData> Renderer::s_SceneData = CreateScope<SceneData>();

    void Renderer::Init()
    {
        RenderCommand::Init();
    }

    void Renderer::Shutdown()
    {
        Renderer3D::Shutdown();
    }

    void Renderer::OnWindowResize(uint32_t width, uint32_t height)
    {
        RenderCommand::SetViewport(0, 0, width, height);
    }

    void Renderer::BeginScene(const Camera& camera)
    {
        s_SceneData->ViewProjectionMatrix = camera.GetViewProjectionMatrix();
    }

    void Renderer::EndScene()
    {
    }

    void Renderer::Submit(const Ref<Shader>& shader, const Ref<VertexArray>& vertexArray, const Mat4& transform)
    {
        using namespace ShaderUniforms;

        shader->Bind();
        shader->SetMat4(ViewProjection, s_SceneData->ViewProjectionMatrix);
        shader->SetMat4(Transform, transform);

        vertexArray->Bind();
        RenderCommand::DrawIndexed(vertexArray);
    }
}
