#pragma once
#include "CBEngine/Renderer/Resources/Texture.h"

namespace CB
{
    class OpenGLTexture2D : public Texture2D
    {
    public:
        OpenGLTexture2D(const String& path);
        OpenGLTexture2D(uint32_t width, uint32_t height, uint32_t color);
        OpenGLTexture2D(uint32_t width, uint32_t height, const void* data, bool nearest);
        ~OpenGLTexture2D() override;

        uint32_t GetWidth() const override { return m_Width; }
        uint32_t GetHeight() const override { return m_Height; }
        uint32_t GetRendererID() const override { return m_RendererID; }

        void Bind(uint32_t slot = 0) const override;

        bool Reload() override;

    private:
        bool LoadFromFile(const String& path);

        String m_Path;
        uint32_t m_Width = 0;
        uint32_t m_Height = 0;
        uint32_t m_RendererID = 0;
        bool m_IsFromFile = false;
    };
}
