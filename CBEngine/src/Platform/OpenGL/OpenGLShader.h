#pragma once
#include "CBEngine/Core/String.h"
#include "CBEngine/Math/CoreMath.h"
#include "CBEngine/Renderer/Resources/Shader.h"

namespace CB
{
    // TODO: REMOVE!
    using GLenum = unsigned int;

    class OpenGLShader : public Shader
    {
    public:
        OpenGLShader(const String& filePath);
        OpenGLShader(const String& name, const String& vertexSrc, const String& fragmentSrc);
        ~OpenGLShader() override;

        void Bind() const override;
        void Unbind() const override;

        const String& GetName() const override { return m_Name; }

        bool Reload() override;

        // Virtual uniform setters (from Shader base class)
        void SetInt(const String& name, int value) override;
        void SetFloat(const String& name, float value) override;
        void SetFloat2(const String& name, const Vector2& value) override;
        void SetFloat3(const String& name, const Vector3& value) override;
        void SetFloat4(const String& name, const Vector4& value) override;
        void SetMat3(const String& name, const Mat3& matrix) override;
        void SetMat4(const String& name, const Mat4& matrix) override;

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
        uint32_t m_RendererID = 0;
        String m_Name;
        String m_FilePath;
        bool m_IsFromFile = false;
    };
}
