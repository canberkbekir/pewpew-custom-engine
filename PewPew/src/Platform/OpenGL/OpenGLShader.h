#pragma once
#include "PewPew/Core/String.h"
#include "PewPew/Math/CoreMath.h"
#include "PewPew/Renderer/Shader.h"

namespace PewPew
{
    // TODO: REMOVE!
    typedef unsigned int GLenum;
    
    class OpenGLShader : public Shader
    {
    public:
        OpenGLShader(const String& filePath); 
        OpenGLShader(const String& name,const String& vertexSrc, const String& fragmentSrc);
        virtual ~OpenGLShader();

        virtual void Bind() const override;
        virtual void Unbind() const override;

        virtual const String& GetName() const override { return m_Name; }

        void UploadUniformInt(const String& name, int value);

        void UploadUniformFloat(const String& name, float value);
        void UploadUniformFloat2(const String& name, const Vector2& value);
        void UploadUniformFloat3(const String& name, const Vector3& value);
        void UploadUniformFloat4(const String& name, const Vector4& value);

        void UploadUniformMat3(const String& name, const Mat3& matrix);
        void UploadUniformMat4(const String& name, const Mat4& matrix);
    private:
        String ReadFile(const String& filepath);
        std::unordered_map<GLenum, String> PreProcess(const String& source);
        void Compile(const std::unordered_map<GLenum, String>& shaderSources);
    private:
        uint32_t m_RendererID;
        String m_Name;
    };

}
