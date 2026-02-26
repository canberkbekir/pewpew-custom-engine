#include "cbpch.h"
#include "Texture.h"

#include "CBEngine/Renderer/Core/Renderer.h"
#include "CBEngine/Renderer/Core/RendererAPI.h"
#include "Platform/OpenGL/OpenGLTexture.h"

namespace CB
{
	Ref<Texture2D> Texture2D::Create(const String path)
	{
		switch (Renderer::GetAPI()) {
		case RendererAPI::API::None: CB_CORE_ASSERT(false, "RendererAPI::None is currently not supported!")
			return nullptr;
		case RendererAPI::API::OpenGL: return CreateRef<OpenGLTexture2D>(path);
		}

		CB_CORE_ASSERT(false, "Unknown RendererAPI!")
		return nullptr;
	}

	Ref<Texture2D> Texture2D::Create(uint32_t width,uint32_t height,uint32_t color)
	{
		switch (Renderer::GetAPI()) {
		case RendererAPI::API::None: CB_CORE_ASSERT(false, "RendererAPI::None is currently not supported!")
			return nullptr;
		case RendererAPI::API::OpenGL: return CreateRef<OpenGLTexture2D>(width, height, color);
		}

		CB_CORE_ASSERT(false, "Unknown RendererAPI!")
		return nullptr;
	}

	Ref<Texture2D> Texture2D::CreateFromData(uint32_t width,uint32_t height,const void* data,bool nearest)
	{
		switch (Renderer::GetAPI()) {
		case RendererAPI::API::None: CB_CORE_ASSERT(false, "RendererAPI::None is currently not supported!")
			return nullptr;
		case RendererAPI::API::OpenGL: return CreateRef<OpenGLTexture2D>(width, height, data, nearest);
		}

		CB_CORE_ASSERT(false, "Unknown RendererAPI!")
		return nullptr;
	}
}