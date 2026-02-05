#include "cbpch.h"
#include "OpenGLTexture.h"

#include "stb_image.h"
#include <glad/glad.h>
#include "CBEngine/Debug/Instrumentor.h"

namespace CB
{
    OpenGLTexture2D::OpenGLTexture2D(const String& path) : m_Path(path), m_IsFromFile(true)
    {
        CB_PROFILE_FUNCTION();
        m_Type = AssetType::Texture2D;
        LoadFromFile(path);
    }

    bool OpenGLTexture2D::LoadFromFile(const String& path)
    {
        CB_PROFILE_FUNCTION();

        int width, height, channels;
        stbi_set_flip_vertically_on_load(1);
        stbi_uc* data = nullptr;
        {
            CB_PROFILE_SCOPE("stbi_load");
            data = stbi_load(path.c_str(), &width, &height, &channels, 0);
        }

        if (!data)
        {
            CB_CORE_ERROR("Failed to load image: {0}", path);
            return false;
        }

        m_Width = width;
        m_Height = height;

        GLenum internalFormat = 0, dataFormat = 0;
        if (channels == 4)
        {
            internalFormat = GL_RGBA8;
            dataFormat = GL_RGBA;
        }
        else if (channels == 3)
        {
            internalFormat = GL_RGB8;
            dataFormat = GL_RGB;
        }
        else if (channels == 2)
        {
            internalFormat = GL_RG8;
            dataFormat = GL_RG;
        }
        else if (channels == 1)
        {
            internalFormat = GL_R8;
            dataFormat = GL_RED;
        }

        if (!(internalFormat && dataFormat))
        {
            CB_CORE_ERROR("Texture format not supported: {0}", path);
            stbi_image_free(data);
            return false;
        }

        glCreateTextures(GL_TEXTURE_2D, 1, &m_RendererID);
        glTextureStorage2D(m_RendererID, 1, internalFormat, m_Width, m_Height);

        glTextureParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_T, GL_REPEAT);

        glTextureSubImage2D(m_RendererID, 0, 0, 0, m_Width, m_Height, dataFormat, GL_UNSIGNED_BYTE, data);

        stbi_image_free(data);
        return true;
    }

    OpenGLTexture2D::OpenGLTexture2D(uint32_t width, uint32_t height, uint32_t color)
        : m_Width(width), m_Height(height), m_IsFromFile(false)
    {
        CB_PROFILE_FUNCTION();
        m_Type = AssetType::Texture2D;

        glCreateTextures(GL_TEXTURE_2D, 1, &m_RendererID);
        glTextureStorage2D(m_RendererID, 1, GL_RGBA8, m_Width, m_Height);

        glTextureParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_T, GL_REPEAT);

        // Fill with solid color
        std::vector<uint32_t> pixels(width * height, color);
        glTextureSubImage2D(m_RendererID, 0, 0, 0, m_Width, m_Height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    }

    OpenGLTexture2D::~OpenGLTexture2D()
    {
        glDeleteTextures(1, &m_RendererID);
    }

    void OpenGLTexture2D::Bind(uint32_t slot) const
    {
        glBindTextureUnit(slot, m_RendererID);
    }

    bool OpenGLTexture2D::Reload()
    {
        if (!m_IsFromFile || m_Path.empty())
        {
            CB_CORE_WARN("Cannot reload texture: not loaded from file");
            return false;
        }

        CB_PROFILE_FUNCTION();

        // Delete old texture
        if (m_RendererID)
        {
            glDeleteTextures(1, &m_RendererID);
            m_RendererID = 0;
        }

        // Reload from file
        if (LoadFromFile(m_Path))
        {
            CB_CORE_INFO("Reloaded texture: {0}", m_Path);
            return true;
        }

        CB_CORE_ERROR("Failed to reload texture: {0}", m_Path);
        return false;
    }
}
