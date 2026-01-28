#pragma once
#include "PewPew/Renderer/Core/RendererAPI.h"
#include "PewPew/Renderer/Resources/VertexArray.h"

namespace PewPew
{
	class OpenGLRendererAPI : public RendererAPI
	{
	public:
		virtual void Init() override;
		virtual void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;
		
		virtual void SetClearColor(const Vector4& color) override;
		virtual void Clear() override;
		
		virtual void DrawIndexed(const Ref<VertexArray>& vertexArray) override;
	};
}
