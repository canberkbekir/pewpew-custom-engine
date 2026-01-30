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
		case RendererAPI::API::None:    PEW_CORE_ASSERT(false, "RendererAPI::None is currently not supported!") return nullptr;
		case RendererAPI::API::OpenGL:  return std::make_shared<OpenGLTexture2D>(path);
		}

		PEW_CORE_ASSERT(false, "Unknown RendererAPI!")

	}

}