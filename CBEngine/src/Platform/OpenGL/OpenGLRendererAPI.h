#pragma once
#include "CBEngine/Renderer/Core/RendererAPI.h"
#include "CBEngine/Renderer/Resources/VertexArray.h"

namespace CB
{
    class OpenGLRendererAPI : public RendererAPI
    {
    public:
        void Init() override;
        void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;

        void SetClearColor(const Vector4& color) override;
        void SetWireframeMode(bool enabled) override;
        void Clear() override;

        void DrawIndexed(const Ref<VertexArray>& vertexArray) override;
    };
}
