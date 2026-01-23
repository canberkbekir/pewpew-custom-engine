#include "pewpch.h"
#include "OpenGLShader.h"

#include <fstream>
#include <vector>
#include <unordered_map>

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

namespace PewPew
{
	namespace
	{
		// Shader source file format expects sections like:
		//   #type vertex
		//   ... GLSL ...
		//   #type fragment
		//   ... GLSL ...
		static GLenum ShaderTypeFromString(const String& type)
		{
			if (type == "vertex")   return GL_VERTEX_SHADER;
			if (type == "fragment") return GL_FRAGMENT_SHADER;
			if (type == "pixel")    return GL_FRAGMENT_SHADER;

			PEW_CORE_ASSERT(false, "Unknown shader type!");
			return 0;
		}

		static String ReadWholeFileBinary(const String& filepath)
		{
			std::ifstream file(filepath, std::ios::in | std::ios::binary);
			if (!file)
			{
				PEW_CORE_ERROR("Could not open file '{0}'", filepath);
				return {};
			}

			file.seekg(0, std::ios::end);
			const std::streamsize size = file.tellg();
			file.seekg(0, std::ios::beg);

			String result;
			result.resize((size_t)size);
			file.read(result.data(), size);
			return result;
		}

		static std::unordered_map<GLenum, String> SplitShaderSourcesByType(const String& source)
		{
			std::unordered_map<GLenum, String> shaderSources;

			const char* token = "#type";
			const size_t tokenLen = strlen(token);

			size_t tokenPos = source.find(token);
			while (tokenPos != String::npos)
			{
				const size_t eol = source.find_first_of("\r\n", tokenPos);
				PEW_CORE_ASSERT(eol != String::npos, "Shader syntax error: missing end-of-line after #type");

				const size_t typeBegin = tokenPos + tokenLen + 1; // skip "#type "
				const String typeStr = source.substr(typeBegin, eol - typeBegin);

				const GLenum stage = ShaderTypeFromString(typeStr);
				PEW_CORE_ASSERT(stage != 0, "Invalid shader type specified");

				const size_t codeBegin = source.find_first_not_of("\r\n", eol);
				PEW_CORE_ASSERT(codeBegin != String::npos, "Shader syntax error: missing shader code after #type");

				const size_t nextTokenPos = source.find(token, codeBegin);
				const size_t codeLen = (nextTokenPos == String::npos) ? String::npos : (nextTokenPos - codeBegin);

				shaderSources[stage] = source.substr(codeBegin, codeLen);
				tokenPos = nextTokenPos;
			}

			return shaderSources;
		}

		static GLuint CompileSingleStage(GLenum stage, const String& source)
		{
			const GLuint shader = glCreateShader(stage);

			const GLchar* src = source.c_str();
			glShaderSource(shader, 1, &src, nullptr);
			glCompileShader(shader);

			GLint compiled = 0;
			glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
			if (compiled == GL_FALSE)
			{
				GLint logLen = 0;
				glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLen);

				std::vector<GLchar> log((size_t)logLen);
				glGetShaderInfoLog(shader, logLen, &logLen, log.data());

				glDeleteShader(shader);

				PEW_CORE_ERROR("{0}", log.data());
				PEW_CORE_ASSERT(false, "Shader compilation failure!");
				return 0;
			}

			return shader;
		}

		static bool LinkProgram(GLuint program)
		{
			glLinkProgram(program);

			GLint linked = 0;
			glGetProgramiv(program, GL_LINK_STATUS, &linked);
			if (linked == GL_FALSE)
			{
				GLint logLen = 0;
				glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLen);

				std::vector<GLchar> log((size_t)logLen);
				glGetProgramInfoLog(program, logLen, &logLen, log.data());

				PEW_CORE_ERROR("{0}", log.data());
				PEW_CORE_ASSERT(false, "Shader link failure!");
				return false;
			}

			return true;
		}
	} // anonymous namespace

	OpenGLShader::OpenGLShader(const String& filePath)
	{
		const String source = ReadWholeFileBinary(filePath);
		const auto shaderSources = SplitShaderSourcesByType(source);
		Compile(shaderSources);
	}

	OpenGLShader::OpenGLShader(const String& vertexSrc, const String& fragmentSrc)
	{
		std::unordered_map<GLenum, String> sources;
		sources[GL_VERTEX_SHADER] = vertexSrc;
		sources[GL_FRAGMENT_SHADER] = fragmentSrc;
		Compile(sources);
	}

	OpenGLShader::~OpenGLShader()
	{
		glDeleteProgram(m_RendererID);
	}

	String OpenGLShader::ReadFile(const String& filepath)
	{
		// Kept for API compatibility; internal helpers do the real work.
		return ReadWholeFileBinary(filepath);
	}

	std::unordered_map<GLenum, String> OpenGLShader::PreProcess(const String& source)
	{
		// Kept for API compatibility; internal helpers do the real work.
		return SplitShaderSourcesByType(source);
	}

	void OpenGLShader::Compile(const std::unordered_map<GLenum, String>& shaderSources)
	{
		const GLuint program = glCreateProgram();

		std::vector<GLuint> compiledStages;
		compiledStages.reserve(shaderSources.size());

		// Compile and attach each stage
		for (const auto& [stage, src] : shaderSources)
		{
			const GLuint shader = CompileSingleStage(stage, src);
			if (shader == 0)
			{
				// Compilation already logged/asserted
				glDeleteProgram(program);
				return;
			}

			glAttachShader(program, shader);
			compiledStages.push_back(shader);
		}

		// Link
		if (!LinkProgram(program))
		{
			// Cleanup compiled stages + program
			for (const GLuint s : compiledStages)
				glDeleteShader(s);

			glDeleteProgram(program);
			return;
		}

		// Detach + delete stages after successful link
		for (const GLuint s : compiledStages)
		{
			glDetachShader(program, s);
			glDeleteShader(s);
		}

		m_RendererID = program;
	}

	void OpenGLShader::Bind() const
	{
		glUseProgram(m_RendererID);
	}

	void OpenGLShader::Unbind() const
	{
		glUseProgram(0);
	}

	// NOTE: These uniform uploads do glGetUniformLocation every call.
	// For performance, consider caching locations in a map (name -> GLint).
	void OpenGLShader::UploadUniformInt(const String& name, int value)
	{
		const GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		glUniform1i(location, value);
	}

	void OpenGLShader::UploadUniformFloat(const String& name, float value)
	{
		const GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		glUniform1f(location, value);
	}

	void OpenGLShader::UploadUniformFloat2(const String& name, const glm::vec2& value)
	{
		const GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		glUniform2f(location, value.x, value.y);
	}

	void OpenGLShader::UploadUniformFloat3(const String& name, const glm::vec3& value)
	{
		const GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		glUniform3f(location, value.x, value.y, value.z);
	}

	void OpenGLShader::UploadUniformFloat4(const String& name, const glm::vec4& value)
	{
		const GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		glUniform4f(location, value.x, value.y, value.z, value.w);
	}

	void OpenGLShader::UploadUniformMat3(const String& name, const glm::mat3& matrix)
	{
		const GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(matrix));
	}

	void OpenGLShader::UploadUniformMat4(const String& name, const glm::mat4& matrix)
	{
		const GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(matrix));
	}

} 
