#pragma once
#include "CBEngine/Renderer/Resources/Buffer.h"

namespace CB
{
	class OpenGLVertexBuffer : public VertexBuffer
	{
	public:
		OpenGLVertexBuffer(float* vertices,uint32_t size);
		OpenGLVertexBuffer(uint32_t size); // Dynamic buffer (GL_DYNAMIC_DRAW)
		~OpenGLVertexBuffer() override;

		void Bind() const override;
		void Unbind() const override;

		const BufferLayout& GetLayout() const override { return m_Layout; }
		void SetLayout(const BufferLayout& layout) override { m_Layout = layout; }
		void SetSubData(const void* data,uint32_t size,uint32_t offset = 0) override;
	private:
		uint32_t m_RendererID;
		BufferLayout m_Layout;
	};

	class OpenGLIndexBuffer : public IndexBuffer
	{
	public:
		OpenGLIndexBuffer(uint32_t* indices,uint32_t count);
		~OpenGLIndexBuffer() override;

		void Bind() const override;
		void Unbind() const override;

		uint32_t GetCount() const override { return m_Count; }
	private:
		uint32_t m_RendererID;
		uint32_t m_Count;
	};
}