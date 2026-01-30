#include "pewpch.h"
#include "VertexArray.h"
#include "PewPew/Renderer/Core/Renderer.h"
#include "PewPew/Renderer/Core/RendererAPI.h"
#include "Platform/OpenGL/OpenGLVertexArray.h"

namespace PewPew
{
    Ref<VertexArray> VertexArray::Create()
    {
        switch (Renderer::GetAPI())
        {
        case RendererAPI::API::None: PEW_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
            return nullptr;
        case RendererAPI::API::OpenGL: return CreateRef<OpenGLVertexArray>();
        }

        PEW_CORE_ASSERT(false, "Unknown RendererAPI!")
        return nullptr;
    }
}
