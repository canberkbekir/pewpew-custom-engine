#include "pewpch.h"
#include "Material.h"
#include "PewPew/Renderer/Core/ShaderUniforms.h"

namespace PewPew
{
    Material::Material() = default;

    Ref<Material> Material::Create()
    {
        return std::make_shared<Material>();
    }

    void Material::Bind(const Ref<Shader>& shader) const
    {
        using namespace ShaderUniforms;

        // Texture slot assignments:
        // 0 = Albedo
        // 1 = Normal
        // 2 = Metallic
        // 3 = Roughness

        // Upload texture presence flags
        shader->SetInt(UseAlbedoMap, m_AlbedoMap ? 1 : 0);
        shader->SetInt(UseNormalMap, m_NormalMap ? 1 : 0);
        shader->SetInt(UseMetallicMap, m_MetallicMap ? 1 : 0);
        shader->SetInt(UseRoughnessMap, m_RoughnessMap ? 1 : 0);

        // Bind textures and set sampler uniforms
        if (m_AlbedoMap)
        {
            m_AlbedoMap->Bind(0);
            shader->SetInt(AlbedoMap, 0);
        }

        if (m_NormalMap)
        {
            m_NormalMap->Bind(1);
            shader->SetInt(NormalMap, 1);
        }

        if (m_MetallicMap)
        {
            m_MetallicMap->Bind(2);
            shader->SetInt(MetallicMap, 2);
        }

        if (m_RoughnessMap)
        {
            m_RoughnessMap->Bind(3);
            shader->SetInt(RoughnessMap, 3);
        }

        // Upload scalar fallbacks
        shader->SetFloat3(Albedo, m_Albedo);
        shader->SetFloat(Metallic, m_Metallic);
        shader->SetFloat(Roughness, m_Roughness);
        shader->SetFloat(SmoothAmount, m_SmoothShading);
    }
}
