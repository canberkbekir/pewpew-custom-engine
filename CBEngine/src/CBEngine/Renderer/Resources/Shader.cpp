#include "cbpch.h"
#include "Shader.h"

#include "CBEngine/Renderer/Core/Renderer.h"
#include "CBEngine/Renderer/Core/RendererAPI.h"
#include "Platform/OpenGL/OpenGLShader.h"

namespace CB
{
	Ref<Shader> Shader::Create(const String& filepath)
	{
		switch (Renderer::GetAPI()) {
		case RendererAPI::API::None: CB_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
			return nullptr;
		case RendererAPI::API::OpenGL: return std::make_shared<OpenGLShader>(filepath);
		}

		CB_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

	Ref<Shader> Shader::Create(const String& name,const String& vertexSrc,const String& fragmentSrc)
	{
		switch (Renderer::GetAPI()) {
		case RendererAPI::API::None: CB_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
			return nullptr;
		case RendererAPI::API::OpenGL: return std::make_shared<OpenGLShader>(name, vertexSrc, fragmentSrc);
		}

		CB_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

	//------------------------------------------------------------ 

	void ShaderLibrary::Add(const String& name,const Ref<Shader>& shader)
	{
		CB_CORE_ASSERT(!Exists(name), "Shader already exists!");
		m_Shaders[name] = shader;
	}

	void ShaderLibrary::Add(const Ref<Shader>& shader)
	{
		auto& name = shader->GetName();
		Add(name, shader);
	}

	Ref<Shader> ShaderLibrary::Load(const String& filepath)
	{
		auto shader = Shader::Create(filepath);
		Add(shader);
		return shader;
	}

	Ref<Shader> ShaderLibrary::Load(const String& name,const String& filepath)
	{
		auto shader = Shader::Create(filepath);
		Add(name, shader);
		return shader;
	}

	Ref<Shader> ShaderLibrary::Get(const String& name)
	{
		CB_CORE_ASSERT(Exists(name), "Shader not found!");
		return m_Shaders[name];
	}

	bool ShaderLibrary::Exists(const String& name) const { return m_Shaders.find(name) != m_Shaders.end(); }
}