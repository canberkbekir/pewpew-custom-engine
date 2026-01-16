#pragma once 
#include "PewPew/Math/CoreMath.h"

namespace PewPew
{
	class Shader
	{
	public:
		Shader(const std::string& vertexSrc,const std::string& fragmentSrc);
		~Shader();

		void Bind();
		void Unbind();

		void UploadUniformMat4(const std::string& uniformName,const Mat4& matrix);
	private:
		uint32_t m_RendererID;
	};

}
