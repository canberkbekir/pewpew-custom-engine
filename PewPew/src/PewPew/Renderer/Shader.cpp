#include "pewpch.h"
#include "Shader.h"  
#include "Renderer.h"
#include "glm/gtc/type_ptr.hpp"
#include "Platform/OpenGL/OpenGLShader.h"

PewPew::Shader* PewPew::Shader::Create(const String& vertexSrc, const String& fragmentSrc)
{
    switch (Renderer::GetAPI())
    {
    case RendererAPI::API::None:    PEW_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
    case RendererAPI::API::OpenGL:  return new OpenGLShader(vertexSrc, fragmentSrc);
    }

    PEW_CORE_ASSERT(false, "Unknown RendererAPI!");
    return nullptr;
}
