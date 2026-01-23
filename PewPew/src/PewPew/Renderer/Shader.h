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
		
		static Shader* Create(const String& filePath); 
		static Shader* Create(const String& vertexSrc, const String& fragmentSrc);
	};

}
