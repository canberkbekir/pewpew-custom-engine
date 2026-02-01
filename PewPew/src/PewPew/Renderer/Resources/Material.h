#pragma once
#include "PewPew/Core/Core.h"
#include "PewPew/Core/String.h"
#include "PewPew/Math/CoreMath.h"
#include "Texture.h"
#include "Shader.h"

namespace PewPew
{
    class Material
    {
    public:
        Material();
        ~Material() = default;

        static Ref<Material> Create();

        // Texture setters
        void SetAlbedoMap(const Ref<Texture2D>& texture) { m_AlbedoMap = texture; }
        void SetNormalMap(const Ref<Texture2D>& texture) { m_NormalMap = texture; }
        void SetMetallicMap(const Ref<Texture2D>& texture) { m_MetallicMap = texture; }
        void SetRoughnessMap(const Ref<Texture2D>& texture) { m_RoughnessMap = texture; }

        // Scalar property setters
        void SetAlbedo(const Vector3& color) { m_Albedo = color; }
        void SetMetallic(float value) { m_Metallic = value; }
        void SetRoughness(float value) { m_Roughness = value; }
        void SetSmoothShading(float value) { m_SmoothShading = value; }

        // Getters
        const Ref<Texture2D>& GetAlbedoMap() const { return m_AlbedoMap; }
        const Ref<Texture2D>& GetNormalMap() const { return m_NormalMap; }
        const Ref<Texture2D>& GetMetallicMap() const { return m_MetallicMap; }
        const Ref<Texture2D>& GetRoughnessMap() const { return m_RoughnessMap; }

        const Vector3& GetAlbedo() const { return m_Albedo; }
        float GetMetallic() const { return m_Metallic; }
        float GetRoughness() const { return m_Roughness; }
        float GetSmoothShading() const { return m_SmoothShading; }

        bool HasAlbedoMap() const { return m_AlbedoMap != nullptr; }
        bool HasNormalMap() const { return m_NormalMap != nullptr; }
        bool HasMetallicMap() const { return m_MetallicMap != nullptr; }
        bool HasRoughnessMap() const { return m_RoughnessMap != nullptr; }

        // Bind all textures and upload uniforms to shader
        void Bind(const Ref<Shader>& shader) const;

    private:
        // Textures
        Ref<Texture2D> m_AlbedoMap;
        Ref<Texture2D> m_NormalMap;
        Ref<Texture2D> m_MetallicMap;
        Ref<Texture2D> m_RoughnessMap;

        // Scalar fallbacks
        Vector3 m_Albedo = {1.0f, 1.0f, 1.0f};
        float m_Metallic = 0.0f;
        float m_Roughness = 0.5f;
        float m_SmoothShading = 1.0f;
    };
}
