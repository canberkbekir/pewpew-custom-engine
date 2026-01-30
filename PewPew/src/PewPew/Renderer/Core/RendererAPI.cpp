#include "pewpch.h"
#include "RendererAPI.h"

#include "Platform/OpenGL/OpenGLRendererAPI.h"


namespace PewPew
{
    RendererAPI::API RendererAPI::s_API = API::OpenGL;

    Scope<RendererAPI> RendererAPI::Create()
    {
        switch (s_API)
        {
        case RendererAPI::API::None:    PEW_CORE_ASSERT(false, "RendererAPI::None is currently not supported!") return nullptr;
        case RendererAPI::API::OpenGL:  return CreateScope<OpenGLRendererAPI>();
        }

        PEW_CORE_ASSERT(false, "Unknown RendererAPI!")
        return nullptr;
    }
}
