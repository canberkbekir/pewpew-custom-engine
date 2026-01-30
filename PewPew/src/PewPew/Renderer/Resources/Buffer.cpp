#include "pewpch.h"
#include "Buffer.h"

#include "PewPew/Renderer/Core/Renderer.h"
#include "PewPew/Renderer/Core/RendererAPI.h"
#include "Platform/OpenGL/OpenGLBuffer.h"

namespace PewPew
{
    Ref<VertexBuffer> VertexBuffer::Create(float* vertices, uint32_t size)
    {
        switch (Renderer::GetAPI())
        {
        case RendererAPI::API::None: PEW_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
            return nullptr;
        case RendererAPI::API::OpenGL: return CreateRef<OpenGLVertexBuffer>(vertices, size);
        }

        PEW_CORE_ASSERT(false, "Unknown RendererAPI!");
        return nullptr;
    }

    Ref<IndexBuffer> IndexBuffer::Create(uint32_t* indices, uint32_t size)
    {
        switch (Renderer::GetAPI())
        {
        case RendererAPI::API::None: PEW_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
            return nullptr;
        case RendererAPI::API::OpenGL: return CreateRef<OpenGLIndexBuffer>(indices, size);
        }

        PEW_CORE_ASSERT(false, "Unknown RendererAPI!")
        return nullptr;
    }
}
