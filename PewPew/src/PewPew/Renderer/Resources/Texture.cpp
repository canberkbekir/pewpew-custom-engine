#include "pewpch.h"
#include "Texture.h"

#include "PewPew/Renderer/Core/Renderer.h"
#include "PewPew/Renderer/Core/RendererAPI.h"
#include "Platform/OpenGL/OpenGLTexture.h"

namespace PewPew
{
    Ref<Texture2D> Texture2D::Create(const String path)
    {
        switch (Renderer::GetAPI())
        {
        case RendererAPI::API::None: PEW_CORE_ASSERT(false, "RendererAPI::None is currently not supported!")
            return nullptr;
        case RendererAPI::API::OpenGL: return CreateRef<OpenGLTexture2D>(path);
        }

        PEW_CORE_ASSERT(false, "Unknown RendererAPI!")
        return nullptr;
    }

    Ref<Texture2D> Texture2D::Create(uint32_t width, uint32_t height, uint32_t color)
    {
        switch (Renderer::GetAPI())
        {
        case RendererAPI::API::None: PEW_CORE_ASSERT(false, "RendererAPI::None is currently not supported!")
            return nullptr;
        case RendererAPI::API::OpenGL: return CreateRef<OpenGLTexture2D>(width, height, color);
        }

        PEW_CORE_ASSERT(false, "Unknown RendererAPI!")
        return nullptr;
    }
}
