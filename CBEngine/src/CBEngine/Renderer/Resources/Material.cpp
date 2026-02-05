#include "cbpch.h"
#include "Material.h"
#include "CBEngine/Renderer/Core/ShaderUniforms.h"
#include "CBEngine/Core/Log.h"

#include <fstream>
#include <sstream>
#include <filesystem>

namespace CB
{
    Material::Material()
    {
        m_Type = AssetType::Material;
    }

    Ref<Material> Material::Create()
    {
        return CreateRef<Material>();
    }

    Ref<Material> Material::Load(const String& filePath)
    {
        Ref<Material> material = CreateRef<Material>();
        if (material->LoadFromFile(filePath))
        {
            material->m_FilePath = filePath;
            material->m_IsFromFile = true;
            material->m_Path = filePath;
            return material;
        }
        return nullptr;
    }

    bool Material::LoadFromFile(const String& filePath)
    {
        std::ifstream file(filePath);
        if (!file.is_open())
        {
            CB_CORE_ERROR("Failed to open material file: {0}", filePath);
            return false;
        }

        // Get directory of material file for relative texture paths
        std::filesystem::path matDir = std::filesystem::path(filePath).parent_path();

        std::string line;
        while (std::getline(file, line))
        {
            // Skip empty lines and comments
            if (line.empty() || line[0] == '#')
                continue;

            // Find key=value
            size_t eqPos = line.find('=');
            if (eqPos == std::string::npos)
                continue;

            std::string key = line.substr(0, eqPos);
            std::string value = line.substr(eqPos + 1);

            // Trim whitespace
            auto trim = [](std::string& s) {
                s.erase(0, s.find_first_not_of(" \t\r\n"));
                s.erase(s.find_last_not_of(" \t\r\n") + 1);
            };
            trim(key);
            trim(value);

            // Parse properties
            if (key == "albedo")
            {
                float r, g, b;
                if (sscanf(value.c_str(), "%f,%f,%f", &r, &g, &b) == 3)
                    m_Albedo = { r, g, b };
            }
            else if (key == "metallic")
            {
                m_Metallic = std::stof(value);
            }
            else if (key == "roughness")
            {
                m_Roughness = std::stof(value);
            }
            else if (key == "smoothShading")
            {
                m_SmoothShading = std::stof(value);
            }
            else if (key == "albedoMap")
            {
                if (!value.empty())
                {
                    m_AlbedoMapPath = value;
                    std::filesystem::path texPath = (matDir / value).lexically_normal();
                    if (std::filesystem::exists(texPath))
                        m_AlbedoMap = Texture2D::Create(texPath.string());
                    else
                        CB_CORE_WARN("Albedo map not found: {0}", texPath.string());
                }
            }
            else if (key == "normalMap")
            {
                if (!value.empty())
                {
                    m_NormalMapPath = value;
                    std::filesystem::path texPath = (matDir / value).lexically_normal();
                    if (std::filesystem::exists(texPath))
                        m_NormalMap = Texture2D::Create(texPath.string());
                    else
                        CB_CORE_WARN("Normal map not found: {0}", texPath.string());
                }
            }
            else if (key == "metallicMap")
            {
                if (!value.empty())
                {
                    m_MetallicMapPath = value;
                    std::filesystem::path texPath = (matDir / value).lexically_normal();
                    if (std::filesystem::exists(texPath))
                        m_MetallicMap = Texture2D::Create(texPath.string());
                    else
                        CB_CORE_WARN("Metallic map not found: {0}", texPath.string());
                }
            }
            else if (key == "roughnessMap")
            {
                if (!value.empty())
                {
                    m_RoughnessMapPath = value;
                    std::filesystem::path texPath = (matDir / value).lexically_normal();
                    if (std::filesystem::exists(texPath))
                        m_RoughnessMap = Texture2D::Create(texPath.string());
                    else
                        CB_CORE_WARN("Roughness map not found: {0}", texPath.string());
                }
            }
        }

        CB_CORE_INFO("Loaded material: {0}", filePath);
        return true;
    }

    bool Material::Save(const String& filePath) const
    {
        std::ofstream file(filePath);
        if (!file.is_open())
        {
            CB_CORE_ERROR("Failed to create material file: {0}", filePath);
            return false;
        }

        file << "# CBEngine Material File\n";
        file << "albedo=" << m_Albedo.x << "," << m_Albedo.y << "," << m_Albedo.z << "\n";
        file << "metallic=" << m_Metallic << "\n";
        file << "roughness=" << m_Roughness << "\n";
        file << "smoothShading=" << m_SmoothShading << "\n";

        if (!m_AlbedoMapPath.empty())
            file << "albedoMap=" << m_AlbedoMapPath << "\n";
        if (!m_NormalMapPath.empty())
            file << "normalMap=" << m_NormalMapPath << "\n";
        if (!m_MetallicMapPath.empty())
            file << "metallicMap=" << m_MetallicMapPath << "\n";
        if (!m_RoughnessMapPath.empty())
            file << "roughnessMap=" << m_RoughnessMapPath << "\n";

        if (file.fail())
        {
            CB_CORE_ERROR("Failed to write material file: {0}", filePath);
            return false;
        }

        CB_CORE_INFO("Saved material: {0}", filePath);
        return true;
    }

    bool Material::Reload()
    {
        if (!m_IsFromFile || m_FilePath.empty())
        {
            CB_CORE_WARN("Cannot reload material: not loaded from file");
            return false;
        }

        // Clear existing textures
        m_AlbedoMap = nullptr;
        m_NormalMap = nullptr;
        m_MetallicMap = nullptr;
        m_RoughnessMap = nullptr;

        if (LoadFromFile(m_FilePath))
        {
            CB_CORE_INFO("Reloaded material: {0}", m_FilePath);
            return true;
        }

        CB_CORE_ERROR("Failed to reload material: {0}", m_FilePath);
        return false;
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
