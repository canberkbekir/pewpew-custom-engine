#pragma once
#include "PewPew/Core/String.h"
#include "PewPew/Math/CoreMath.h"
#include "PewPew/Renderer/Resources/Shader.h"

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

        // Virtual uniform setters (from Shader base class)
        virtual void SetInt(const String& name, int value) override;
        virtual void SetFloat(const String& name, float value) override;
        virtual void SetFloat2(const String& name, const Vector2& value) override;
        virtual void SetFloat3(const String& name, const Vector3& value) override;
        virtual void SetFloat4(const String& name, const Vector4& value) override;
        virtual void SetMat3(const String& name, const Mat3& matrix) override;
        virtual void SetMat4(const String& name, const Mat4& matrix) override;

        // OpenGL-specific upload methods (kept for backwards compatibility)
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
