#pragma once 
#include "PewPew/Core/String.h" 

namespace PewPew
{
	class Shader
	{
	public:
		virtual ~Shader() = default;

		virtual void Bind() const = 0;
		virtual void Unbind() const = 0;

		virtual const String& GetName() const = 0;
		
		static Ref<Shader> Create(const String& filePath); 
		static Ref<Shader> Create(const String& name, const String& vertexSrc, const String& fragmentSrc);
	};

	class ShaderLibrary
	{
	public:
		void Add(const String& name, const Ref<Shader>& shader);
		void Add(const Ref<Shader>& shader);
		Ref<Shader> Load(const String& filepath);
		Ref<Shader> Load(const String& name, const String& filepath);

		Ref<Shader> Get(const String& name);

		bool Exists(const String& name) const;
	private:
		std::unordered_map<String, Ref<Shader>> m_Shaders;
	};

}
