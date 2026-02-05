#include "cbpch.h"
#include "VertexArray.h"
#include "CBEngine/Renderer/Core/Renderer.h"
#include "CBEngine/Renderer/Core/RendererAPI.h"
#include "Platform/OpenGL/OpenGLVertexArray.h"

namespace CB
{
    Ref<VertexArray> VertexArray::Create()
    {
        switch (Renderer::GetAPI())
        {
        case RendererAPI::API::None: CB_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
            return nullptr;
        case RendererAPI::API::OpenGL: return CreateRef<OpenGLVertexArray>();
        }

        CB_CORE_ASSERT(false, "Unknown RendererAPI!")
        return nullptr;
    }
}
