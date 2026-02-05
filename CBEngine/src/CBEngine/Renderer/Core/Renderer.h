#pragma once
#include "CBEngine/Renderer/Camera/Camera.h"
#include "CBEngine/Renderer/Core/RendererAPI.h"
#include "CBEngine/Renderer/Resources/Shader.h"
#include "CBEngine/Renderer/Resources/VertexArray.h"

namespace CB
{
    class Renderer
    {
    public:
        static void Init();
        static void Shutdown();
        static void OnWindowResize(uint32_t width, uint32_t height);

        static void BeginScene(const Camera& camera);
        static void EndScene();

        // Basic submit (for simple rendering without lighting)
        static void Submit(const Ref<Shader>& shader, const Ref<VertexArray>& vertexArray,
                           const Mat4& transform = Mat4(1.0f));

        static RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }

    private:
        struct SceneData
        {
            Mat4 ViewProjectionMatrix;
        };

        static Scope<SceneData> s_SceneData;
    };
}
