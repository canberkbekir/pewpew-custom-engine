#pragma once
#include "PewPew/Renderer/Resources/Texture.h"

namespace PewPew
{
    class OpenGLTexture2D : public Texture2D
    {
    public:
        OpenGLTexture2D(const String& path);
        OpenGLTexture2D(uint32_t width, uint32_t height, uint32_t color);
        ~OpenGLTexture2D() override;

        uint32_t GetWidth() const override { return m_Width; }
        uint32_t GetHeight() const override { return m_Height; }
        uint32_t GetRendererID() const override { return m_RendererID; }

        void Bind(uint32_t slot = 0) const override;

    private:
        String m_Path;
        uint32_t m_Width, m_Height;
        uint32_t m_RendererID;
    };
}
