#pragma once

#include "RendererAPI.h"

namespace CB
{
	class RenderCommand
	{
	public:
		static void Init() { s_RendererAPI->Init(); }

		static void SetViewport(uint32_t x,uint32_t y,uint32_t width,uint32_t height)
		{
			s_RendererAPI->SetViewport(x, y, width, height);
		}

		static void SetClearColor(const Vector4& color) { s_RendererAPI->SetClearColor(color); }

		static void SetWireframeMode(bool enabled) { s_RendererAPI->SetWireframeMode(enabled); }

		static void Clear() { s_RendererAPI->Clear(); }

		static void DrawIndexed(const Ref<VertexArray>& vertexArray) { s_RendererAPI->DrawIndexed(vertexArray); }

		static void DrawIndexedInstanced(const Ref<VertexArray>& vertexArray,uint32_t instanceCount)
		{
			s_RendererAPI->DrawIndexedInstanced(vertexArray, instanceCount);
		}
	private:
		static Scope<RendererAPI> s_RendererAPI;
	};
}