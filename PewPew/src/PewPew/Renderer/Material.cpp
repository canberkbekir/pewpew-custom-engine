#include "pewpch.h"
#include "Material.h"

namespace PewPew
{
	Material::Material() = default;

	Ref<Material> Material::Create()
	{
		return std::make_shared<Material>();
	}

	void Material::Bind(const Ref<Shader>& shader) const
	{
		// Texture slot assignments:
		// 0 = Albedo
		// 1 = Normal
		// 2 = Metallic
		// 3 = Roughness

		// Upload texture presence flags
		shader->SetInt("u_UseAlbedoMap", m_AlbedoMap ? 1 : 0);
		shader->SetInt("u_UseNormalMap", m_NormalMap ? 1 : 0);
		shader->SetInt("u_UseMetallicMap", m_MetallicMap ? 1 : 0);
		shader->SetInt("u_UseRoughnessMap", m_RoughnessMap ? 1 : 0);

		// Bind textures and set sampler uniforms
		if (m_AlbedoMap)
		{
			m_AlbedoMap->Bind(0);
			shader->SetInt("u_AlbedoMap", 0);
		}

		if (m_NormalMap)
		{
			m_NormalMap->Bind(1);
			shader->SetInt("u_NormalMap", 1);
		}

		if (m_MetallicMap)
		{
			m_MetallicMap->Bind(2);
			shader->SetInt("u_MetallicMap", 2);
		}

		if (m_RoughnessMap)
		{
			m_RoughnessMap->Bind(3);
			shader->SetInt("u_RoughnessMap", 3);
		}

		// Upload scalar fallbacks
		shader->SetFloat3("u_Albedo", m_Albedo);
		shader->SetFloat("u_Metallic", m_Metallic);
		shader->SetFloat("u_Roughness", m_Roughness);

		// Shading properties
		shader->SetFloat("u_SmoothAmount", m_SmoothAmount);
		shader->SetFloat("u_StylizedAmount", m_StylizedAmount);
		shader->SetInt("u_ShaderMode", m_ShaderMode);
		shader->SetFloat("u_PixelSize", m_PixelSize);
		shader->SetFloat("u_VoxelSize", m_VoxelSize);
	}
}
